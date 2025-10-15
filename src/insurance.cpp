#include "insurance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>


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
    // TODO
    unpaidBills.push_back({who,bill});
}

void Insurance::payBills() {

    for(auto it = unpaidBills.begin(); it != unpaidBills.end(); ++it){
        if(money >= it->second){
            it->first->pay(unpaidBills[x].second);
            it = unpaidBills.erase(it);
        }
    }
    // TODO
}
