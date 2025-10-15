// tests/test_hospital.cpp
#include <gtest/gtest.h>
#include <pcosynchro/pcothread.h>
#include <memory>
#include <vector>

#include "hospital.h"
#include "clinic.h"
#include "insurance.h"
#include "seller.h"
#include "costs.h"
#include "day_clock.h"

// --------- Doubles / wrappers ---------

// Assurance qui paie immédiatement le bénéficiaire (utile pour voir l'argent arriver)
class ImmediateInsurance : public Seller {
public:
    ImmediateInsurance(int id, int fund = 0) : Seller(fund, id) {}
    void invoice(int bill, Seller* who) override { who->pay(bill); }
    int  transfer(ItemType, int) override { return 0; }
    int  buy(ItemType, int) override { return 0; }
    void pay(int) override {}
};

// "Clinique" qui accepte tout patient malade qu'on lui envoie.
class AcceptAllClinic : public Seller {
public:
    AcceptAllClinic(int id, int fund = 0) : Seller(fund, id) {}
    int transfer(ItemType what, int qty) override {
        if (what != ItemType::SickPatient) return 0;
        accepted += qty;
        stocks[ItemType::SickPatient] += qty;
        return qty;
    }
    int  buy(ItemType, int) override { return 0; }
    void invoice(int, Seller*) override {}
    void pay(int) override {}
    int getAccepted() const { return accepted; }
private:
    int accepted = 0;
};


// Expose quelques éléments pour test
class TestableHospital : public Hospital {
public:
    using Hospital::Hospital;
    using Hospital::transferSickPatientsToClinic;
    using Hospital::updateRehab;
    using Hospital::payNursingStaff;
    using Hospital::setClinics;
    using Hospital::setInsurance;
    using Hospital::setClock;
    using Hospital::nbNursingStaff;
    using Hospital::nbFreed;
    using Seller::money;
    using Seller::stocks;
    using Seller::nbEmployeesPaid;
    using Hospital::getNumberPatients;
};

class HospitalFixture : public ::testing::Test {
protected:
    void SetUp() override {
        hosp   = std::make_unique<TestableHospital>(/*id*/1, /*fund*/100'000, /*maxBeds*/50);
        ins    = std::make_unique<ImmediateInsurance>(2, 0);
        clinic = std::make_unique<AcceptAllClinic>(3, 0);

        hosp->setInsurance(ins.get());
        hosp->setClinics({clinic.get()});

        // État propre
        hosp->stocks[ItemType::SickPatient]  = 0;
        hosp->stocks[ItemType::RehabPatient] = 0;
    }


    // Membres
    std::unique_ptr<TestableHospital> hosp;
    std::unique_ptr<ImmediateInsurance> ins;
    std::unique_ptr<AcceptAllClinic> clinic;

};


// --------- Tests ---------
TEST_F(HospitalFixture, ReceivesSickPatientsRespectingFundsAndBeds) {
    TestableHospital local(10, /*fund*/1'000, /*maxBeds*/5);

    TestableHospital poor(11, /*fund*/0, /*maxBeds*/5);
    EXPECT_EQ(poor.transfer(ItemType::SickPatient, 3), 0);

    EXPECT_EQ(local.transfer(ItemType::SickPatient, 10), 5);
    EXPECT_EQ(local.stocks[ItemType::SickPatient], 5);

    EXPECT_EQ(local.transfer(ItemType::SickPatient, 2), 0);
}

TEST_F(HospitalFixture, ReceivesRehabPatientsStartsTimers) {
    int got = hosp->transfer(ItemType::RehabPatient, 3);
    EXPECT_EQ(got, 3);
    EXPECT_EQ(hosp->stocks[ItemType::RehabPatient], 3);

    int startFund = hosp->money;
    for (int day = 0; day < 5; ++day) hosp->updateRehab();

    EXPECT_EQ(hosp->stocks[ItemType::RehabPatient], 0);
    EXPECT_EQ(hosp->getNumberPatients(), 3);

    int rehabPrice = getCostPerService(ServiceType::Rehab);
    EXPECT_EQ(hosp->money, startFund + 3 * rehabPrice);
}

TEST_F(HospitalFixture, payNursingStaffDeductsMoneyAndCountsEmployees) {
    int salaryPer = getEmployeeSalary(EmployeeType::NursingStaff);
    int startFund = hosp->money;
    int startPaid = hosp->nbEmployeesPaid;

    hosp->payNursingStaff();

    EXPECT_EQ(hosp->money, startFund - hosp->nbNursingStaff * salaryPer);
    EXPECT_EQ(hosp->nbEmployeesPaid, startPaid + hosp->nbNursingStaff);
}

TEST_F(HospitalFixture, TransferSickPatientsToClinicMovesSomeAndInvoicesInsurance) {
    EXPECT_EQ(hosp->transfer(ItemType::SickPatient, 20), 20);
    int startSick = hosp->stocks[ItemType::SickPatient];
    int startFund = hosp->money;

    for (int i = 0; i < 20; ++i) {
        hosp->transferSickPatientsToClinic();
        if (clinic->getAccepted() > 0) break;
    }

    ASSERT_GT(clinic->getAccepted(), 0);
    int moved = clinic->getAccepted();
    int price = getCostPerService(ServiceType::PreTreatmentStay);

    EXPECT_EQ(hosp->stocks[ItemType::SickPatient], startSick - moved);
    EXPECT_EQ(hosp->money, startFund + moved * price);
}


TEST(HospitalPay, PayIsThreadSafeAndIncreasesFunds) {
    TestableHospital hosp(40, 0, 5);
    const int N = 1000;
    std::vector<std::unique_ptr<PcoThread>> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back(std::make_unique<PcoThread>([&]() {
            for (int k = 0; k < N; ++k) hosp.pay(1);
        }));
    }
    for (auto& t : ts) t->join();
    EXPECT_EQ(hosp.money, 4 * N);
}

TEST_F(HospitalFixture, EndToEndShortRunDoesMeaningfulWork) {
    EXPECT_EQ(hosp->transfer(ItemType::SickPatient, 15), 15);
    EXPECT_EQ(hosp->transfer(ItemType::RehabPatient, 5), 5);

    int startFund  = hosp->money;
    int startSick  = hosp->stocks[ItemType::SickPatient];
    int startRehab = hosp->stocks[ItemType::RehabPatient];

    DayClock clock(/*participants*/1);
    hosp->setClock(&clock);

    std::unique_ptr<PcoThread> th = std::make_unique<PcoThread>([&](){ hosp->run(); });

    for (int d = 0; d < 5; ++d) {
        clock.start_next_day();
        clock.wait_all_done();
    }

    th->requestStop();
    clock.start_next_day();
    th->join();

    int endFund = hosp->money;
    endFund -= clinic->getAccepted() * getCostPerService(ServiceType::PreTreatmentStay);
    endFund -= hosp->nbFreed * getCostPerService(ServiceType::Rehab);
    endFund += hosp->getAmountPaidToEmployees(EmployeeType::NursingStaff);

    EXPECT_LE(hosp->stocks[ItemType::SickPatient], startSick);
    EXPECT_LE(hosp->stocks[ItemType::RehabPatient], startRehab);
    EXPECT_EQ(endFund, startFund);
}
