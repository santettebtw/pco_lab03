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
        if (PcoThread::thisThread()->stopRequested()) break;

		// wait for money to be high enough to pay employee salary before sending patient(s)
		// wait if stock[ItemType::SickPatient] == 0
		while (money < getEmployeeSalary(EmployeeType::EmergencyStaff) || stocks[ItemType::SickPatient] == 0) { }
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

	// decrease stock[ItemType::SickPatient] and only send max patients possible
	nbPatientsToTransfer = nbPatientsToTransfer > stocks[ItemType::SickPatient] ? stocks[ItemType::SickPatient] : nbPatientsToTransfer;
	stocks[ItemType::SickPatient] -= nbPatientsToTransfer; // TODO critique
	hospital->transfer(ItemType::SickPatient, nbPatientsToTransfer);
	insurance->invoice(getEmployeeSalary(EmployeeType::EmergencyStaff), this);
	// increment number of employees payed with a mutex and decrease money, only if possible
	nbEmployeesPaid++; // TODO critique
	money -= getEmployeeSalary(EmployeeType::EmergencyStaff); // TODO critique
}

void Ambulance::pay(int bill) {
	money += bill; // TODO critique
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
