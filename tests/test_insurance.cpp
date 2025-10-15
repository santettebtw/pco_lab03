// tests/test_insurance.cpp
#include <gtest/gtest.h>
#include <pcosynchro/pcothread.h>
#include <memory>
#include <vector>

#include "insurance.h"
#include "seller.h"
#include "costs.h"
#include "day_clock.h" 

// --------- Doubles de test ---------

// Un vendeur qui enregistre ce qu'il reçoit (pour vérifier que l'assurance paie bien)
class PayRecorderSeller : public Seller {
public:
    PayRecorderSeller(int id, int fund = 0) : Seller(fund, id) {}
    void pay(int bill) override {
        received += bill;
        money += bill; // simule un encaissement réel
        return;
    }
    int transfer(ItemType, int) override { return 0; }
    int buy(ItemType, int) override { return 0; }
    void invoice(int, Seller*) override { return; }

    int getReceived() const { return received; }
    int getFund() const { return money; }

private:
    PcoMutex mx;
    int received = 0;
};

// Wrapper testable pour exposer payBills() et money
class TestableInsurance : public Insurance {
public:
    using Insurance::Insurance;
    using Insurance::payBills;
    using Seller::money;
};

// --------- Tests ---------

TEST(InsuranceBasic, InvoiceQueuesBills_AndPayBillsPaysWhenFundsSufficient) {
    TestableInsurance ins(1, /*fund*/0);
    PayRecorderSeller clinic(2), hospital(3);

    // Deux factures en attente
    ins.invoice(300, &clinic);
    ins.invoice(200, &hospital);

    // Sans fonds : rien ne se passe
    ins.payBills();
    EXPECT_EQ(clinic.getReceived(), 0);
    EXPECT_EQ(hospital.getReceived(), 0);

    // On crédite l'assurance puis on paie
    ins.money = 1'000;
    ins.payBills();

    EXPECT_EQ(clinic.getReceived(), 300);
    EXPECT_EQ(hospital.getReceived(), 200);
    EXPECT_EQ(ins.money, 1'000 - 500);
}

TEST(InsuranceThreading, ConcurrentInvoicesAccumulateThenGetPaid) {
    TestableInsurance ins(10, /*fund*/0);
    PayRecorderSeller clinic(11);

    // 4 threads poussent des factures en parallèle
    const int N = 500;
    std::vector<std::unique_ptr<PcoThread>> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back(std::make_unique<PcoThread>([&]() {
            for (int k = 0; k < N; ++k) {
                ins.invoice(1, &clinic);
            }
        }));
    }
    for (auto& t : ts) t->join();

    // On crédite juste ce qu'il faut pour tout payer
    ins.money = 4 * N;
    ins.payBills();

    EXPECT_EQ(clinic.getReceived(), 4 * N);
    EXPECT_EQ(ins.money, 0);
}

TEST(InsuranceRun, RunPaysBillsInLoop) {
    TestableInsurance ins(20, /*fund*/0);
    PayRecorderSeller hosp(21);

    // Préparer des factures (total = 100)
    for (int i = 0; i < 10; ++i) ins.invoice(10, &hosp);

    DayClock clock(/*participants*/1);
    ins.setClock(&clock);

    ins.money = -10;

    std::unique_ptr<PcoThread> th = std::make_unique<PcoThread>([&](){ ins.run(); });

    // Jour 0 : pas assez d'argent → rien ne se paie
    clock.start_next_day();
    clock.wait_all_done();
    EXPECT_EQ(hosp.getReceived(), 0);

    // Créditer puis déclencher une nouvelle journée
    ins.money = 100;
    clock.start_next_day();
    clock.wait_all_done();

    // Arrêt propre
    th->requestStop();
    clock.start_next_day();
    th->join();

    EXPECT_EQ(hosp.getReceived(), 100);
    EXPECT_EQ(ins.money, 10);
}
