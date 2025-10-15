// tests/test_clinic.cpp
#include <gtest/gtest.h>
#include <pcosynchro/pcothread.h>
#include <random>
#include <memory>
#include <vector>
#include <map>

#include "clinic.h"
#include "seller.h"
#include "supplier.h"
#include "costs.h"
#include "day_clock.h" 

// ---------- Wrappers testables ----------

class TestableClinic : public Clinic {
public:
    using Clinic::Clinic;
    using Clinic::payBills;
    using Clinic::sendPatientsToRehab;
    using Clinic::orderResources;
    using Clinic::treatOne;
    using Seller::stocks;
    using Seller::money;
    using Clinic::nbEmployeesPaid;
    using Clinic::unpaidBills;

    void setHospitalsAndSuppliers(std::vector<Seller*> hs, std::vector<Seller*> sups) {
        Clinic::setHospitalsAndSuppliers(std::move(hs), std::move(sups));
    }
    void setInsurance(Seller* ins) { Clinic::setInsurance(ins); }

    void setFunds(int v) { money = v; }
    void setPatients(int sick, int rehab=0) {
        stocks[ItemType::SickPatient] = sick;
        stocks[ItemType::RehabPatient] = rehab;
    }
    void setResource(ItemType r, int qty) { stocks[r] = qty; }
    int employeesPaid() const { return nbEmployeesPaid; }
};

// ---------- Doubles de test ----------

class RehabHospital : public Seller {
public:
    RehabHospital(int id, int fund, int capacity = 1'000'000)
        : Seller(fund, id), capacity(capacity) {}

    int transfer(ItemType what, int qty) override {
        if (what != ItemType::RehabPatient) return 0;
        int free = capacity - admitted;
        int acc = std::max(0, std::min(qty, free));
        admitted += acc;
        stocks[ItemType::RehabPatient] += acc;
        return acc;
    }

    int  buy(ItemType, int) override { throw std::logic_error("Hospital::buy not used"); }
    void invoice(int, Seller*) override { throw std::logic_error("Hospital::invoice not used"); }
    void pay(int) override { throw std::logic_error("Hospital::pay not used"); }

    int getAdmitted() const { return admitted; }

private:
    int capacity;
    int admitted = 0;
};

class ImmediateInsurance : public Seller {
public:
    ImmediateInsurance(int id, int fund) : Seller(fund, id) {}
    void invoice(int bill, Seller* who) override { who->pay(bill); return;  }
    int  transfer(ItemType, int) override { throw std::logic_error("Insurance::transfer not used"); }
    int  buy(ItemType, int) override { throw std::logic_error("Insurance::buy not used"); }
    void pay(int) override { throw std::logic_error("Insurance::pay not used"); }
};

// Supplier de test CONFORME : on ne change pas la logique d’achat/paiement
class TestSupplier : public Supplier {
public:
    TestSupplier(int id, int fund, std::vector<ItemType> catalog)
        : Supplier(id, fund, std::move(catalog)) {}

    using Seller::stocks;
    using Seller::money;
    using Seller::nbEmployeesPaid;

    void setStock(ItemType it, int qty) { stocks[it] = qty; }
    int getStock(ItemType it) const {
        auto itf = stocks.find(it);
        return itf == stocks.end() ? 0 : itf->second;
    }
    int getFund() const { return money; }
};

// ---------- Fixture ----------

class ClinicFixture : public ::testing::Test {
protected:
    void SetUp() override {
        clinic = std::make_unique<TestableClinic>(1, /*fund*/10'000,
                          std::vector<ItemType>{ItemType::Pill, ItemType::Thermometer});
        hosp = std::make_unique<RehabHospital>(2, 0, /*capacity*/1000);
        ins  = std::make_unique<ImmediateInsurance>(3, 0);

        supA = std::make_unique<TestSupplier>(10, 0, std::vector<ItemType>{ItemType::Pill});
        supB = std::make_unique<TestSupplier>(11, 0, std::vector<ItemType>{ItemType::Thermometer});

        // Précharger du stock pour que Supplier::buy réussisse
        supA->setStock(ItemType::Pill, 10);
        supB->setStock(ItemType::Thermometer, 10);

        clinic->setHospitalsAndSuppliers({hosp.get()}, {supA.get(), supB.get()});
        clinic->setInsurance(ins.get());

        clinic->setResource(ItemType::SickPatient, 0);
        clinic->setResource(ItemType::RehabPatient, 0);
        clinic->setResource(ItemType::Pill, 0);
        clinic->setResource(ItemType::Thermometer, 0);
    }

    std::unique_ptr<TestableClinic> clinic;
    std::unique_ptr<RehabHospital> hosp;
    std::unique_ptr<ImmediateInsurance> ins;
    std::unique_ptr<TestSupplier> supA, supB;
};

// ---------- Tests ----------

TEST_F(ClinicFixture, OrderResources_BuysMissing_OnNoStock) {
    EXPECT_EQ(clinic->stocks[ItemType::Pill], 0);
    EXPECT_EQ(clinic->stocks[ItemType::Thermometer], 0);

    clinic->orderResources();

    EXPECT_EQ(clinic->stocks[ItemType::Pill], 1);
    EXPECT_EQ(clinic->stocks[ItemType::Thermometer], 1);

    // Les stocks fournisseurs ont bien décru
    EXPECT_EQ(supA->getStock(ItemType::Pill), 9);
    EXPECT_EQ(supB->getStock(ItemType::Thermometer), 9);
}

TEST_F(ClinicFixture, HasResourcesThenTreatOne_ConsumesResourcesAndMovesPatientToRehab) {
    const int salary = getEmployeeSalary(EmployeeType::TreatmentSpecialist);
    clinic->setFunds(salary * 10);
    clinic->setPatients(/*sick*/1, /*rehab*/0);
    clinic->setResource(ItemType::Pill, 1);
    clinic->setResource(ItemType::Thermometer, 1);

    clinic->treatOne();

    EXPECT_EQ(clinic->stocks[ItemType::SickPatient], 0);
    EXPECT_EQ(clinic->stocks[ItemType::RehabPatient], 1);
    EXPECT_EQ(clinic->stocks[ItemType::Pill], 0);
    EXPECT_EQ(clinic->stocks[ItemType::Thermometer], 0);
    EXPECT_EQ(clinic->employeesPaid(), 1);
}

TEST_F(ClinicFixture, TreatOne_Fails_WithoutMoney) {
    clinic->setFunds(0);
    clinic->setPatients(1, 0);
    clinic->setResource(ItemType::Pill, 1);
    clinic->setResource(ItemType::Thermometer, 1);

    clinic->treatOne();

    EXPECT_EQ(clinic->stocks[ItemType::SickPatient], 1);
    EXPECT_EQ(clinic->stocks[ItemType::RehabPatient], 0);
    EXPECT_EQ(clinic->employeesPaid(), 0);
}

TEST_F(ClinicFixture, SendPatientsToRehab_InvoicesInsuranceAndReducesRehabStock) {
    const int price = getCostPerService(ServiceType::Treatment);

    clinic->setPatients(0, /*rehab*/5);
    int startAdmitted = hosp->getAdmitted();
    int startFund = clinic->money;

    clinic->sendPatientsToRehab();

    int admitted = hosp->getAdmitted() - startAdmitted;
    ASSERT_GT(admitted, 0);

    EXPECT_EQ(clinic->stocks[ItemType::RehabPatient], 5 - admitted);
    EXPECT_EQ(clinic->money, startFund + admitted * price);
}

TEST_F(ClinicFixture, Transfer_AcceptsSickPatientsOnly_AndOnlyIfNoDebtAndNoUnpaidBills) {
    clinic->setFunds(1);
    int got = clinic->transfer(ItemType::SickPatient, 3);
    EXPECT_EQ(got, 3);
    EXPECT_EQ(clinic->stocks[ItemType::SickPatient], 3);

    EXPECT_EQ(clinic->transfer(ItemType::Pill, 2), 0);

    clinic->setResource(ItemType::Pill, 0);
    clinic->setResource(ItemType::Thermometer, 0);
    clinic->orderResources();
    EXPECT_EQ(clinic->transfer(ItemType::SickPatient, 1), 0);

    TestableClinic poor(99, /*fund*/0, {ItemType::Pill});
    poor.setHospitalsAndSuppliers({hosp.get()}, {supA.get()});
    poor.setInsurance(ins.get());
    EXPECT_EQ(poor.transfer(ItemType::SickPatient, 5), 0);
}

TEST_F(ClinicFixture, PayBills_PaysWhenFundsSufficient) {
    // Force des factures (manque les 2 ressources)
    clinic->setResource(ItemType::Pill, 0);
    clinic->setResource(ItemType::Thermometer, 0);
    clinic->orderResources();

    int startClinic = clinic->money;
    int startSupA = supA->getFund();
    int startSupB = supB->getFund();

    clinic->pay(1'000);
    clinic->payBills();

    int billA = getCostPerUnit(ItemType::Pill);
    int billB = getCostPerUnit(ItemType::Thermometer);

    EXPECT_EQ(supA->getFund(), startSupA + billA);
    EXPECT_EQ(supB->getFund(), startSupB + billB);
    EXPECT_EQ(clinic->money, startClinic + 1'000 - (billA + billB));
}

TEST(ClinicUnsupported, TransferNonSickReturnsZero_BuyAndInvoiceThrowIfCalled) {
    TestableClinic c(5, 100, {ItemType::Pill});
    EXPECT_THROW(c.buy(ItemType::Pill, 1), std::logic_error);
    EXPECT_THROW(c.invoice(10, &c), std::logic_error);
    EXPECT_NO_THROW(c.pay(1));
}

TEST(ClinicPay, PayIsThreadSafeAndIncreasesFunds) {
    TestableClinic c(6, 0, {ItemType::Pill});
    const int N = 1000;
    std::vector<std::unique_ptr<PcoThread>> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back(std::make_unique<PcoThread>([&]() {
            for (int k = 0; k < N; ++k) c.pay(1);
        }));
    }
    for (auto& t : ts) t->join();
    EXPECT_EQ(c.getFund(), 4 * N);
}

TEST_F(ClinicFixture, Run_EndToEnd_TreatsAndTransfersAtLeastOnce) {
    clinic->setPatients(10, 0);
    clinic->setFunds(10'000);
    clinic->setResource(ItemType::Pill, 0);
    clinic->setResource(ItemType::Thermometer, 0);
    
    int startAdmitted = hosp->getAdmitted();
    int startFund = clinic->getFund();

    DayClock clock(/*participants*/1);
    clinic->setClock(&clock);

    std::unique_ptr<PcoThread> th = std::make_unique<PcoThread>([&](){ clinic->run(); });

    // Faire quelques journées pour laisser le flux s’exécuter :
    // payBills -> (éventuel) orderResources -> treatOne -> sendPatientsToRehab
    for (int d = 0; d < 5; ++d) {
        clock.start_next_day();
        clock.wait_all_done();
    }

    th->requestStop();
    clock.start_next_day();
    th->join();

    int endFund = clinic->getFund();
    endFund += (5 * (6 + 5)); // prix matériel commandé
    endFund += clinic->getAmountPaidToEmployees(EmployeeType::TreatmentSpecialist); // salaires payés
    endFund -= (hosp->getAdmitted() - startAdmitted) * getCostPerService(ServiceType::Treatment); // factures envoyées

    EXPECT_GT(hosp->getAdmitted(), startAdmitted);
    EXPECT_EQ(endFund, startFund);
}
