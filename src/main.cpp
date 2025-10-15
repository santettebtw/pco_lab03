// main.cpp (headless)
#include <iostream>
#include <memory>
#include <vector>
#include <pcosynchro/pcothread.h>

#include "ambulance.h"
#include "supplier.h"
#include "clinic.h"
#include "hospital.h"
#include "insurance.h"

#include "day_clock.h"
#include "utils.h"


int main(int argc, char **argv) {
    int NB_DAYS;
    int NB_SUPPLIER;
    int NB_INSURANCE;
    int NB_CLINICS;
    int NB_HOSPITALS;
    int NB_AMBULANCE;

    // Valeurs par défaut
    const int DEFAULT_DAYS = 6;
    const int DEFAULT_SUPPLIER = 3;
    const int DEFAULT_INSURANCE = 1;
    const int DEFAULT_CLINIC = 3;
    const int DEFAULT_HOSPITAL = 2;
    const int DEFAULT_AMBULANCE = 2;

    // Si aucun argument supplémentaire : utiliser les valeurs par défaut
    if (argc == 1) {
        NB_DAYS      = DEFAULT_DAYS;
        NB_SUPPLIER  = DEFAULT_SUPPLIER;
        NB_INSURANCE = DEFAULT_INSURANCE;
        NB_CLINICS   = DEFAULT_CLINIC;
        NB_HOSPITALS = DEFAULT_HOSPITAL;
        NB_AMBULANCE = DEFAULT_AMBULANCE;
    }
    else if (argc == 2) {
        NB_DAYS = atoi(argv[1]);
        NB_SUPPLIER  = DEFAULT_SUPPLIER;
        NB_INSURANCE = DEFAULT_INSURANCE;
        NB_CLINICS   = DEFAULT_CLINIC;
        NB_HOSPITALS = DEFAULT_HOSPITAL;
        NB_AMBULANCE = DEFAULT_AMBULANCE;
    }
    // Si le nombre de paramètres est incorrect
    else if (argc != 7) {
        printf("Usage: %s NB_DAYS\n or\n", argv[0]);
        printf("Usage: %s NB_DAYS NB_SUPPLIER NB_INSURANCE NB_CLINIC NB_HOSPITAL NB_AMBULANCE\n", argv[0]);
        return 1;
    }
    // Sinon : lire les valeurs depuis argv
    else {
        NB_DAYS      = atoi(argv[1]);
        NB_SUPPLIER  = atoi(argv[2]);
        NB_INSURANCE = DEFAULT_INSURANCE;
        NB_CLINICS   = atoi(argv[4]);
        NB_HOSPITALS = atoi(argv[5]);
        NB_AMBULANCE = atoi(argv[6]);
    }

    auto ambulances = createAmbulances(NB_AMBULANCE, 0);
    auto suppliers  = createSuppliers(NB_SUPPLIER, NB_AMBULANCE);
    auto hospitals  = createHospitals(NB_HOSPITALS, NB_AMBULANCE + NB_SUPPLIER);
    auto clinics    = createClinics(NB_CLINICS, NB_AMBULANCE + NB_HOSPITALS + NB_SUPPLIER);

    Insurance insurance(NB_AMBULANCE + NB_HOSPITALS + NB_CLINICS + NB_SUPPLIER + NB_INSURANCE, INSURANCE_FUND);

    std::vector<Seller*> sellersHospitals;
    sellersHospitals.reserve(hospitals.size());
    for (auto* h : hospitals) sellersHospitals.push_back(h);

    std::vector<Seller*> sellersSuppliers;
    sellersSuppliers.reserve(suppliers.size());
    for (auto* s : suppliers) sellersSuppliers.push_back(s);

    int clinicsByHospital = NB_CLINICS / NB_HOSPITALS;
    int clinicsShared     = NB_CLINICS % NB_HOSPITALS;
    int countClinic = 0;

    for (auto & hospital : hospitals) {
        std::vector<Seller*> tmpClinics;
        int start = countClinic;
        int end   = countClinic + clinicsByHospital;
        for (int k = start; k < end && k < (int)clinics.size(); ++k)
            tmpClinics.push_back(clinics[k]);
        // partage
        for (int k = NB_CLINICS - clinicsShared; k < NB_CLINICS; ++k)
            if (k >= 0 && k < (int)clinics.size())
                tmpClinics.push_back(clinics[k]);

        hospital->setClinics(tmpClinics);
        hospital->setInsurance(&insurance);
        countClinic += clinicsByHospital;
    }

    for (auto* a : ambulances) {
        a->setHospitals(sellersHospitals);
        a->setInsurance(&insurance);
    }

    for (auto* c : clinics) {
        c->setHospitalsAndSuppliers(sellersHospitals, sellersSuppliers);
        c->setInsurance(&insurance);
    }

    const int PARTICIPANTS =
        (int)ambulances.size() +
        (int)suppliers.size()  +
        (int)clinics.size()    +
        (int)hospitals.size()  + 
        NB_INSURANCE; // +1 pour l'assurance


    DayClock clock(PARTICIPANTS);

    for (auto* a : ambulances) a->setClock(&clock);
    for (auto* s : suppliers)  s->setClock(&clock);
    for (auto* c : clinics)    c->setClock(&clock);
    for (auto* h : hospitals)  h->setClock(&clock);
    insurance.setClock(&clock);

    std::vector<std::unique_ptr<PcoThread>> threads;
    threads.reserve(ambulances.size() + suppliers.size() + clinics.size() + hospitals.size() + NB_INSURANCE);

    for (auto* a : ambulances) threads.emplace_back(std::make_unique<PcoThread>(&Ambulance::run, a));
    for (auto* s : suppliers)  threads.emplace_back(std::make_unique<PcoThread>(&Supplier::run,  s));
    for (auto* c : clinics)    threads.emplace_back(std::make_unique<PcoThread>(&Clinic::run,    c));
    for (auto* h : hospitals)  threads.emplace_back(std::make_unique<PcoThread>(&Hospital::run,  h));
    threads.emplace_back(std::make_unique<PcoThread>(&Insurance::run, &insurance));

    for (int d = 0; d < NB_DAYS; ++d) {
        clock.start_next_day(); // “jour d” commence pour tout le monde
        clock.wait_all_done();  // attend que tous aient fini leur journée
    }

    // Stop les threads
    endService(threads);

    clock.start_next_day(); // Libère les potentiels worker bloqué

    for (auto& t : threads) t->join();

    int startPatient = INITIAL_PATIENT_SICK;
    int endPatient   = 0;

    int startFund = (SUPPLIER_FUND * static_cast<int>(ambulances.size())) +
                    (SUPPLIER_FUND * static_cast<int>(suppliers.size())) +
                    (CLINICS_FUND  * static_cast<int>(clinics.size())) +
                    (HOSPITALS_FUND* static_cast<int>(hospitals.size())) +
                    INSURANCE_FUND;

    int endFund = 0;

    for (Ambulance* a : ambulances) {
        int ambulanceFinalFund = a->getFund();
        std::cout << "Final fund for ambulance is : " << ambulanceFinalFund  << "\n";
        endFund += ambulanceFinalFund;

        int ambulanceAmountPaidToEmployees = a->getAmountPaidToEmployees(EmployeeType::EmergencyStaff);
        std::cout << "Final amount paid for employees for ambulance is : " << ambulanceAmountPaidToEmployees << "\n\n";
        endFund += ambulanceAmountPaidToEmployees;

        endPatient += a->getNumberPatients();
    }
    for (Supplier* s : suppliers) {
        int supplierFinalFund = s->getFund();
        std::cout << "Final fund for supplier is : " << supplierFinalFund  << "\n";
        endFund += supplierFinalFund;

        int supplierAmountPaidToEmployees = s->getAmountPaidToEmployees(EmployeeType::Supplier);
        std::cout << "Final amount paid for employees for supplier is : " << supplierAmountPaidToEmployees << "\n\n";
        endFund += supplierAmountPaidToEmployees;
    }
    for (Clinic* c : clinics) {
        int clinicFinalFund = c->getFund();
        std::cout << "Final fund for clinic is : " << clinicFinalFund  << "\n";
        endFund += clinicFinalFund;

        int clinicAmountPaidToEmployees = c->getAmountPaidToEmployees(EmployeeType::TreatmentSpecialist);
        std::cout << "Final amount paid for employees for clinic is : " << clinicAmountPaidToEmployees << "\n\n";
        endFund += clinicAmountPaidToEmployees;

        endPatient += c->getNumberPatients();
    }
    for (Hospital* h : hospitals) {
        int hospitalFinalFund = h->getFund();
        std::cout << "Final fund for hospital is : " << hospitalFinalFund  << "\n";
        endFund += hospitalFinalFund;

        int hospitalAmountPaidToEmployees = h->getAmountPaidToEmployees(EmployeeType::NursingStaff);
        std::cout << "Final amount paid for employees for hospital is : " << hospitalAmountPaidToEmployees << "\n\n";
        endFund += hospitalAmountPaidToEmployees;

        endPatient += h->getNumberPatients();
    }
    int insuranceFinalFund = insurance.getFund();
    std::cout << "Final fund for insurance is : " << insuranceFinalFund  << "\n\n\n\n";
    endFund += insuranceFinalFund - (INSURANCE_CONTRIBUTION * NB_DAYS);

    std::cout << "The expected fund is : " << startFund << " and you got at the end : " << endFund << "\n";
    std::cout << "The expected patient is : " << startPatient << " and you got at the end : " << endPatient << "\n";

    return 0;
}
