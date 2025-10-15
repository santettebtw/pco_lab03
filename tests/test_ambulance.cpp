// tests/test_ambulance.cpp
#include <gtest/gtest.h>
#include <pcosynchro/pcothread.h>
#include <pcosynchro/pcomutex.h>
#include <memory>
#include <vector>
#include "ambulance.h"
#include "seller.h"
#include "costs.h"

// ---------- Testable wrappers ----------

class TestableAmbulance : public Ambulance {
public:
    using Ambulance::Ambulance;
    using Ambulance::sendPatients;   
    using Seller::stocks;            
    using Seller::money;           
    using Ambulance::nbEmployeesPaid;

    void setHospitals(std::vector<Seller*> hs) { Ambulance::setHospitals(std::move(hs)); }
    void setInsurance(Seller* ins) { Ambulance::setInsurance(ins); }

    // utilitaires de test
    void setFunds(int v) { money = v; }
    void setPatients(int v) { stocks[ItemType::SickPatient] = v; }
    [[nodiscard]] int employeesPaid() const { return nbEmployeesPaid; }
};

// Hôpital de test qui accepte tout.
class AcceptAllHospital : public Seller {
public:
    explicit AcceptAllHospital(int id, int fund, int capacity = 1'000'000)
        : Seller(fund, id), capacity(capacity) {}

    int transfer(ItemType what, int qty) override {
        if (what != ItemType::SickPatient) return 0;
        // PcoMutexLocker lock(&mx);
        int free = capacity - admitted;
        int acc = std::max(0, std::min(qty, free));
        admitted += acc;
        stocks[ItemType::SickPatient] += acc;
        return acc;
    }

    int  buy(ItemType, int) override { throw std::logic_error("Hospital::buy not used"); }
    void invoice(int, Seller*) override { throw std::logic_error("Hospital::invoice not used"); }
    void pay(int) override { throw std::logic_error("Hospital::pay not used"); }

    [[nodiscard]] int getAdmitted() const { return admitted; }

private:
    PcoMutex mx;
    int capacity;
    int admitted = 0;
};

// Assurance de test qui paie immédiatement le bénéficiaire (thread-safe)
class ImmediateInsurance : public Seller {
public:
    ImmediateInsurance(int id, int fund) : Seller(fund, id) {}

    void invoice(int bill, Seller* who) override {
        who->pay(bill);
        return ; // crédite directement
    }

    int  buy(ItemType, int) override { throw std::logic_error("Insurance::buy not supported"); }
    int  transfer(ItemType, int) override { throw std::logic_error("Insurance::transfer not supported"); }
    void pay(int) override { throw std::logic_error("Insurance::pay not supported"); }

private:
    PcoMutex mx;
};

// ---------- Fixture ----------

class AmbulanceFixture : public ::testing::Test {
protected:
    void SetUp() override {
        amb = std::make_unique<TestableAmbulance>(
            /*id*/1, /*fund*/10'000,
            std::vector<ItemType>{ItemType::SickPatient},
            std::map<ItemType,int>{{ItemType::SickPatient, 10}}
        );
        hosp = std::make_unique<AcceptAllHospital>(2, 0, /*capacity*/100);
        ins  = std::make_unique<ImmediateInsurance>(3, 0);
        std::vector<Seller*> hs{hosp.get()};
        amb->setHospitals(hs);
        amb->setInsurance(ins.get());
    }

    std::unique_ptr<TestableAmbulance> amb;
    std::unique_ptr<AcceptAllHospital> hosp;
    std::unique_ptr<ImmediateInsurance> ins;
};

// ---------- Tests fonctionnels ----------

TEST_F(AmbulanceFixture, SendPatients_Success) {
    const int startPatients = amb->getNumberPatients();
    const int startFund     = amb->getFund();

    amb->sendPatients();

    const int endPatients = amb->getNumberPatients();
    const int endFund     = amb->getFund();

    EXPECT_LT(endPatients, startPatients);   
    EXPECT_GT(hosp->getAdmitted(), 0);       
    EXPECT_NE(endFund, startFund); 
}


TEST_F(AmbulanceFixture, SendPatients_Fails_When_NoPatients) {
    TestableAmbulance a(10, 10'000, {ItemType::SickPatient}, {{ItemType::SickPatient, 0}});
    std::vector<Seller*> hs{hosp.get()};
    a.setHospitals(hs);
    a.setInsurance(ins.get());

    const int startPatients = a.getNumberPatients();
    const int startFund     = a.getFund();

    a.sendPatients();

    EXPECT_EQ(a.getNumberPatients(), startPatients);
    EXPECT_EQ(a.getFund(), startFund);
    EXPECT_EQ(hosp->getAdmitted(), 0);
}


TEST_F(AmbulanceFixture, SendPatients_Fails_When_NotEnoughMoneyForSalary) {
    TestableAmbulance a(11, /*fund*/0, {ItemType::SickPatient}, {{ItemType::SickPatient, 10}});
    std::vector<Seller*> hs{hosp.get()};
    a.setHospitals(hs);
    a.setInsurance(ins.get());

    const int startPatients = a.getNumberPatients();
    const int startFund     = a.getFund();

    a.sendPatients();

    EXPECT_EQ(a.getNumberPatients(), startPatients);
    EXPECT_EQ(a.getFund(), startFund);
    EXPECT_EQ(hosp->getAdmitted(), 0);
}

TEST_F(AmbulanceFixture, PaySuccess) {
    const int startFund = amb->getFund();

    amb->pay(500);

    const int endFund = amb->getFund();

    EXPECT_EQ(startFund + 500, endFund);
}

TEST_F(AmbulanceFixture, FundsDeltaMatchesSalaryAndTransportBill_OnSuccess) {
    // Δmoney = -salary + transferred * price
    const int salary = getEmployeeSalary(EmployeeType::EmergencyStaff);
    const int price  = getCostPerService(ServiceType::Transport);

    hosp = std::make_unique<AcceptAllHospital>(22, 0, 1000);
    std::vector<Seller*> hs{hosp.get()};
    amb->setHospitals(hs);

    const int startFund     = amb->getFund();
    const int startPatients = amb->getNumberPatients();
    const int startAdmitted = hosp->getAdmitted();

    for (int i = 0; i < 20; ++i) {
        amb->sendPatients();
        if (hosp->getAdmitted() > startAdmitted) break;
    }

    const int transferred = hosp->getAdmitted() - startAdmitted;
    ASSERT_GT(transferred, 0) << "Aucun transfert après 20 essais";

    const int expectedDelta = -salary + transferred * price;
    const int endFund = amb->getFund();

    EXPECT_EQ(endFund - startFund, expectedDelta);
    EXPECT_EQ(amb->getNumberPatients(), startPatients - transferred);
}

TEST(AmbulanceThrows, UnsupportedMethodsThrow) {
    Ambulance a(1, 0, {ItemType::SickPatient}, {{ItemType::SickPatient, 0}});
    EXPECT_THROW(a.buy(ItemType::Pill, 1), std::logic_error);
    EXPECT_THROW(a.transfer(ItemType::Pill, 1), std::logic_error);
    EXPECT_THROW(a.invoice(10, &a), std::logic_error);
    EXPECT_NO_THROW(a.pay(1));
}

TEST(AmbulancePay, PayIsThreadSafeAndIncreasesFunds) {
    TestableAmbulance a(1, /*fund*/0, {ItemType::SickPatient}, {{ItemType::SickPatient, 0}});
    const int N = 1000;
    std::vector<std::unique_ptr<PcoThread>> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back(std::make_unique<PcoThread>([&]() {
            for (int k = 0; k < N; ++k) a.pay(1);
        }));
    }
    for (auto& t : ts) t->join();
    EXPECT_EQ(a.getFund(), 4 * N);
}

