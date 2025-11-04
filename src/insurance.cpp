#include "insurance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>


PcoMutex mutex_ins;

Insurance::Insurance(int uniqueId, int fund) : Seller(fund, uniqueId) {}

void Insurance::run() {
    logger() << "Insurance " <<  uniqueId << " starting with fund " << money << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (PcoThread::thisThread()->stopRequested()) break;

        // Réception de la somme des cotisations journalières des assurés
        receiveContributions();

        // Payer les factures
        payBills();

        clock->worker_end_day();
    }

    logger() << "Insurance " <<  uniqueId << " stopping with fund " << money << std::endl;
}

void Insurance::receiveContributions() {
    money += INSURANCE_CONTRIBUTION;
}

void Insurance::invoice(int bill, Seller* who) {
    mutex_ins.lock();
    unpaidBills.push_back({who,bill});
    mutex_ins.unlock();
}

void Insurance::payBills() {
	mutex_ins.lock();
	for(auto it = unpaidBills.begin(); it != unpaidBills.end(); ){
		if(money >= it->second){
			int billAmount = it->second;
			Seller* beneficiary = it->first; // NOTE: save before erase, was using invalid iterator after erase
			money -= billAmount;
			it = unpaidBills.erase(it);
			mutex_ins.unlock(); // NOTE: similar to Clinic::payBills()
			
			beneficiary->pay(billAmount);
			
			mutex_ins.lock(); // relock mutex before looping back
		} else {
			++it; // only increment if we don't pay the bill
		}
	}
	mutex_ins.unlock();
}
