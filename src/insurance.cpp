#include "insurance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>


Insurance::Insurance(int uniqueId, int fund) : Seller(fund, uniqueId) {}

void Insurance::run() {
    logger() << "Insurance " <<  uniqueId << " starting with fund " << money << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (false /* TODO condition d'arrêt */) break;

        // Réception de la somme des cotisations journalières des assurés
        receiveContributions();

        // Payer les factures
        payBills();

        clock->worker_end_day();
    }

    logger() << "Insurance " <<  uniqueId << " stopping with fund " << money << std::endl;
}

void Insurance::receiveContributions() {

    // TODO

}

void Insurance::invoice(int bill, Seller* who) {

    // TODO

}

void Insurance::payBills() {

    // TODO
}
