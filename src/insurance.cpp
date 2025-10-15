#include "insurance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>


PcoMutex mutex;

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
    mutex.lock();
    unpaidBills.push_back({who,bill});
    mutex.unlock();
}

void Insurance::payBills() {

    for(auto it = unpaidBills.begin(); it != unpaidBills.end(); ++it){
        if(money >= it->second){
            it->first->pay(it->second);
            mutex.lock();
            money -= it->second;
            mutex.unlock();
            it = unpaidBills.erase(it);
        }
    }
    // TODO
}
