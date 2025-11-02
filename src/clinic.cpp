#include "clinic.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include <random>

PcoMutex Mutex_clinic;


Clinic::Clinic(int id, int fund, std::vector<ItemType> resourcesNeeded)
: Seller(fund, id), resourcesNeeded(std::move(resourcesNeeded)) {
    for (auto it : this->resourcesNeeded) {
        stocks[it] = 0;
    }

    stocks[ItemType::SickPatient] = 0;
    stocks[ItemType::RehabPatient] = 0;
}

void Clinic::run() {
    logger() << "Clinic " <<  uniqueId << " starting with fund " << money << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (PcoThread::thisThread()->stopRequested()) break;

        // Essayer de traiter le prochain patient
        processNextPatient();

        // Transférer les patients déjà traités vers un hôpital pour leur réhabilitation
        sendPatientsToRehab();

        // Payer les factures en retard
        payBills();

        clock->worker_end_day();
    }

    logger() << "Clinic " <<  uniqueId << " stopping with fund " << money << std::endl;
}


int Clinic::transfer(ItemType what, int qty) {

    if(what != ItemType::SickPatient){
        return 0;
    }
    int salary = getEmployeeSalary(EmployeeType::TreatmentSpecialist);
    int total = 0;
    for(int x = 0; x < qty; ++x){
        total += salary;
        if(money < total){
            return x;
        }
    }

    return qty;

}

bool Clinic::hasResourcesForTreatment() const {
    for (const auto& [item, quantity] : stocks) {
        if (quantity == 0) {
            return false;
        }
    }
    return true;

}

void Clinic::payBills() {
    for(auto it = unpaidBills.begin(); it != unpaidBills.end(); ++it){
        if(money >= it->second){
            it->first->pay(it->second);
            Mutex_clinic.lock();
            money -= it->second;
            Mutex_clinic.unlock();
            it = unpaidBills.erase(it);
        }
    }
}

void Clinic::processNextPatient() {
    if(hasResourcesForTreatment() && money >=  getEmployeeSalary(EmployeeType::TreatmentSpecialist)){
        treatOne();
    }else{
        orderResources();
    }
}

void Clinic::sendPatientsToRehab() {
    if (!hospitals.empty()) {
        hospitals[0]->transfer(ItemType::RehabPatient, stocks[ItemType::RehabPatient]);
        stocks[ItemType::RehabPatient] = 0;
        insurance->invoice(getTreatmentCost(), this);
    }
}

void Clinic::orderResources() {

    for (auto& [item, quantity] : stocks) {
        if (quantity != 0) continue;

        Supplier *supplier = chooseRandomSupplier(item);
        int bill = supplier->buy(item, 1);
        unpaidBills.push_back({supplier, bill});
    }

}

void Clinic::treatOne() {

    // TODO
    stocks[ItemType::Stethoscope]--;
    stocks[ItemType::Thermometer]--;
    stocks[ItemType::Scalpel]--;
    stocks[ItemType::Pill]--;
    stocks[ItemType::Syringe]--;
    stocks[ItemType::SickPatient]--;
    stocks[ItemType::RehabPatient]++;
}

void Clinic::pay(int bill) {
    this->money += bill;

}

Supplier *Clinic::chooseRandomSupplier(ItemType item) {
    std::vector<Supplier*> availableSuppliers;

    // Sélectionner les Suppliers qui ont la ressource recherchée
    for (Seller* seller : suppliers) {
        auto* sup = dynamic_cast<Supplier*>(seller);
        if (sup->sellsResource(item)) {
            availableSuppliers.push_back(sup);
        }
    }

    // Choisir aléatoirement un Supplier dans la liste
    assert(availableSuppliers.size());
    std::vector<Supplier*> out;
    std::sample(availableSuppliers.begin(), availableSuppliers.end(), std::back_inserter(out),
            1, std::mt19937{std::random_device{}()});
    return out.front();
}

void Clinic::setHospitalsAndSuppliers(std::vector<Seller*> hospitals, std::vector<Seller*> suppliers) {
    this->hospitals = hospitals;
    this->suppliers = suppliers;
}

void Clinic::setInsurance(Seller* ins) { 
    insurance = ins; 
}


int Clinic::getTreatmentCost() {
    return 0;
}

int Clinic::getWaitingPatients() {
    return stocks[ItemType::SickPatient];
}

int Clinic::getNumberPatients() {
    return stocks[ItemType::SickPatient] + stocks[ItemType::RehabPatient];
}

Pulmonology::Pulmonology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::Pill, ItemType::Thermometer}) {}

Cardiology::Cardiology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::Syringe, ItemType::Stethoscope}) {}

Neurology::Neurology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::Pill, ItemType::Scalpel}) {}
