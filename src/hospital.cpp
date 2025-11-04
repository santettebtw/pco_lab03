// hospital.cpp
#include "hospital.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>

PcoMutex Mutex_hospital;

Hospital::Hospital(int id, int fund, int maxBeds)
: Seller(fund, id), maxBeds(maxBeds), nbNursingStaff(maxBeds) {
    stocks[ItemType::SickPatient] = 0;
    stocks[ItemType::RehabPatient] = 0;
}

void Hospital::run() {
    logger() << "Hospital " <<  uniqueId << " starting with fund " << money << ", maxBeds " << maxBeds << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (PcoThread::thisThread()->stopRequested()) break;

        transferSickPatientsToClinic();
        updateRehab();
        payNursingStaff();

        clock->worker_end_day();
    }

    logger() << "Hospital " <<  uniqueId << " stopping with fund " << money << std::endl;
}

void Hospital::transferSickPatientsToClinic() {
	if (stocks[ItemType::SickPatient] == 0) return;
	
	auto* clinic = chooseRandomSeller(clinics);
	
	Mutex_hospital.lock();
    // assuming we transfer all sick patients to the clinic
	int nbPatientsToTransfer = stocks[ItemType::SickPatient];
	Mutex_hospital.unlock();
	
	// try to transfer to clinic (clinic may reject if it doesn't have funds)
	int accepted = clinic->transfer(ItemType::SickPatient, nbPatientsToTransfer);
	
	if (accepted > 0) {
		Mutex_hospital.lock();
		stocks[ItemType::SickPatient] -= accepted; 
		Mutex_hospital.unlock();
		
		insurance->invoice(getCostPerService(ServiceType::PreTreatmentStay) * accepted, this);
	}
}

void Hospital::updateRehab() {
	Mutex_hospital.lock();
	
	// batches to free
	std::vector<int> batchesToFree;
	
	// decrement days for all batches
	for (auto& batch : rehabBatches) {
		batch.second--;
		if (batch.second <= 0) {
			batchesToFree.push_back(batch.first);
		}
	}
	
	// process completed batches
	for (int freedCount : batchesToFree) {
		nbFreed += freedCount; 
		stocks[ItemType::RehabPatient] -= freedCount; 
	}
	
	// remove completed batches
	for (auto it = rehabBatches.begin(); it != rehabBatches.end(); ) {
		if (it->second <= 0) {
			it = rehabBatches.erase(it);
		} else {
			++it;
		}
	}
	
	Mutex_hospital.unlock();
	
	// invoice insurance for rehabilitation, no need to lock this
	for (int freedCount : batchesToFree) {
		insurance->invoice(getCostPerService(ServiceType::Rehab) * freedCount, this);
	}
}

void Hospital::payNursingStaff() {
	int salaryPerStaff = getEmployeeSalary(EmployeeType::NursingStaff);
	int totalSalary = nbNursingStaff * salaryPerStaff;
	
	Mutex_hospital.lock();
	money -= totalSalary;
	nbEmployeesPaid += nbNursingStaff;
	Mutex_hospital.unlock();
}

void Hospital::pay(int bill) {
	Mutex_hospital.lock();
	money += bill;
	Mutex_hospital.unlock();
}

int Hospital::transfer(ItemType what, int qty) {
	Mutex_hospital.lock();
	// don't accept transfers if hospital is in debt
	if (money <= 0) {
		Mutex_hospital.unlock();
		return 0;
	}
	// calculate available beds
	int currentPatients = stocks[ItemType::SickPatient] + stocks[ItemType::RehabPatient];
	int availableBeds = maxBeds - currentPatients;
	Mutex_hospital.unlock();
	
	if (availableBeds <= 0) {
		return 0;
	}
	
	int accepted = qty > availableBeds ? availableBeds : qty;
	
	if (what == ItemType::SickPatient) {
		Mutex_hospital.lock();
		stocks[ItemType::SickPatient] += accepted;
		Mutex_hospital.unlock();
		
		return accepted;
	} else if (what == ItemType::RehabPatient) {
		Mutex_hospital.lock();
		stocks[ItemType::RehabPatient] += accepted;
		// add new batch with 5 days remaining
		rehabBatches.push_back({accepted, 5});
		Mutex_hospital.unlock();
		
		return accepted;
	}
	return 0;
}

int Hospital::getNumberPatients() {
    return stocks[ItemType::SickPatient] + stocks[ItemType::RehabPatient] + nbFreed;
}

void Hospital::setClinics(std::vector<Seller*> c) {
    clinics = std::move(c);
}

void Hospital::setInsurance(Seller* ins) { 
    insurance = ins; 
}
