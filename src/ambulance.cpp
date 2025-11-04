// ambulance.cpp
#include "ambulance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <pcosynchro/pcomutex.h>

PcoMutex Mutex_ambulance;


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
	// check if we can pay employee salary before doing anything
	Mutex_ambulance.lock();
	if (hospitals.empty() || money < getEmployeeSalary(EmployeeType::EmergencyStaff) || stocks[ItemType::SickPatient] == 0) {
		Mutex_ambulance.unlock();
		return;
	}
	
	// Déterminer le nombre de patients à envoyer
	int nbPatientsToTransfer = 1 + rand() % 5;
	// only send max patients possible
	nbPatientsToTransfer = nbPatientsToTransfer > stocks[ItemType::SickPatient] ? stocks[ItemType::SickPatient] : nbPatientsToTransfer;
	Mutex_ambulance.unlock();
	
	// Choisir un hôpital au hasard
	auto* hospital = chooseRandomSeller(hospitals);
	int accepted = hospital->transfer(ItemType::SickPatient, nbPatientsToTransfer);
	
	if (accepted > 0) {
		Mutex_ambulance.lock();
		stocks[ItemType::SickPatient] -= accepted;
		money -= getEmployeeSalary(EmployeeType::EmergencyStaff);
		nbEmployeesPaid++;
		Mutex_ambulance.unlock();
		
		// invoice insurance for transport cost (per patient)
		insurance->invoice(getCostPerService(ServiceType::Transport) * accepted, this);
	}
}

void Ambulance::pay(int bill) {
	Mutex_ambulance.lock();
	money += bill;
	Mutex_ambulance.unlock();
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
