// ambulance.cpp
#include "ambulance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>


Ambulance::Ambulance(int id, int fund,
                     std::vector<ItemType> resourcesSupplied,
                     std::map<ItemType,int> initialStocks)
: Seller(fund, id), resourcesSupplied(resourcesSupplied) {
    for (auto it : resourcesSupplied) {
        stocks[it] = initialStocks.count(it) ? initialStocks[it] : 0;
    }
}

void Ambulance::run() {
    logger() << "Ambulance " <<  uniqueId << " starting with fund " << money << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (false /* TODO condition d'arrêt */) break;

        sendPatients();

        clock->worker_end_day();
    }

    logger() << "Ambulance " <<  uniqueId << " stopping with fund " << money << std::endl;
}

void Ambulance::sendPatients() {
    // Choisir un hôpital au hasard
    auto* hospital = chooseRandomSeller(hospitals);
    // Déterminer le nombre de patients à envoyer
    int nbPatientsToTransfer = 1 + rand() % 5;

    // TODO

}

void Ambulance::pay(int bill) {

    // TODO

}

void Ambulance::setHospitals(std::vector<Seller*> h) {
    hospitals = std::move(h);
}

void Ambulance::setInsurance(Seller* ins) { 
    insurance = ins; 
}

int Ambulance::getNumberPatients() {
    return stocks[ItemType::SickPatient];
}
