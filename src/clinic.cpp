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
    
    Mutex_clinic.lock();
    // NOTE: cannot accept if clinic had unpaid bills or is in debt (money <= 0, like hospital)
    if (money <= 0 || !unpaidBills.empty()) {
        Mutex_clinic.unlock();
        return 0;
    }
    
    stocks[ItemType::SickPatient] += qty; // NOTE: was missing, need to update stocks
    Mutex_clinic.unlock();
    
    return qty;
}

bool Clinic::hasResourcesForTreatment() const {
    // NOTE: only check resourcesNeeded items, not all stocks
    // (SickPatient/RehabPatient don't need to be checked)
    for (const auto& item : resourcesNeeded) {
        auto it = stocks.find(item);
        if (it == stocks.end() || it->second == 0) {
            return false;
        }
    }
    return true;
}

void Clinic::payBills() {
    // NOTE: we need to lock BEFORE loop to protect all clinic data (money, unpaidBills) 
    // throughout the entire loop
    Mutex_clinic.lock();
    for(auto it = unpaidBills.begin(); it != unpaidBills.end(); ){
        if(money >= it->second){
            int billAmount = it->second;
            Supplier* supplier = it->first;
            money -= billAmount;
            
            // NOTE: to avoid deadlock as we are calling supplier->pay() which will also aquire a mutex,
            // so we have to release Mutex_clinic first. after unlock, we're safe to modify the iterator (erase) and
            // call external methods. after supplier->pay() is done, we have to relock Mutex_clinic to continue the loop.
            Mutex_clinic.unlock();
            
            it = unpaidBills.erase(it);
            supplier->pay(billAmount);
            
            Mutex_clinic.lock();
        } else {
            ++it;
        }
    }
    Mutex_clinic.unlock();  // release mutex after loop completes
}

void Clinic::processNextPatient() {
    Mutex_clinic.lock();
    bool hasUnpaidBills = !unpaidBills.empty(); // access in lock, action outside
    Mutex_clinic.unlock();
    
    if (!hasUnpaidBills) { // NOTE: cannot order if unpaid bills exist
        orderResources();
    }
    
    // after ordering (if possible), try to treat if we have resources and funds
    Mutex_clinic.lock();
    // access in lock, action outside
    bool canTreat = hasResourcesForTreatment() && money >= getEmployeeSalary(EmployeeType::TreatmentSpecialist);
    Mutex_clinic.unlock();
    
    if(canTreat){
        treatOne();
    }
}

void Clinic::sendPatientsToRehab() {
    if (!hospitals.empty()) {
        Mutex_clinic.lock();
        int numPatients = stocks[ItemType::RehabPatient];
        Mutex_clinic.unlock();
        
        if (numPatients > 0) {
            int transferred = hospitals[0]->transfer(ItemType::RehabPatient, numPatients);
            
            if (transferred > 0) {
                Mutex_clinic.lock();
                stocks[ItemType::RehabPatient] -= transferred;
                Mutex_clinic.unlock();
                
                // NOTE: invoice per patient transferred, not just once
                insurance->invoice(getTreatmentCost() * transferred, this);
            }
        }
    }
}

void Clinic::orderResources() {
    Mutex_clinic.lock();
    // NOTE: only iterate over resourcesNeeded, not all stocks (which includes SickPatient/RehabPatient)
    for (const auto& item : resourcesNeeded) {
        if (stocks[item] != 0) continue;
        Mutex_clinic.unlock();
        
        Supplier *supplier = chooseRandomSupplier(item);
        int bill = supplier->buy(item, 1);
        
        Mutex_clinic.lock();
        if (bill > 0) { // NOTE: only update stock and add bill if purchase was successful
            stocks[item] += 1; // NOTE: was missing, need to update clinic stock after buying
            unpaidBills.push_back({supplier, bill});
        }
    }
    Mutex_clinic.unlock();
}

void Clinic::treatOne() {
    Mutex_clinic.lock();
    int salary = getEmployeeSalary(EmployeeType::TreatmentSpecialist);
    
    if (money < salary) {
        Mutex_clinic.unlock();
        return;
    }
    
    money -= salary; // NOTE: was missing, must pay salary before treatment
    nbEmployeesPaid++; // NOTE: was missing, must increment after each patient treatment
    
    // NOTE: update stocks, only consume resources that are actually needed
    for (const auto& item : resourcesNeeded) {
        stocks[item]--;
    }
    stocks[ItemType::SickPatient]--;
    stocks[ItemType::RehabPatient]++;
    Mutex_clinic.unlock();
}

void Clinic::pay(int bill) {
    Mutex_clinic.lock(); // NOTE: we needed ot lock here
    this->money += bill;
    Mutex_clinic.unlock();
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
    // NOTE: was returning 0, should return actual treatment service cost
    return getCostPerService(ServiceType::Treatment);
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
