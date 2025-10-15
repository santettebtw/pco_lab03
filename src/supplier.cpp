#include "supplier.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>


Supplier::Supplier(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied) {
    for (const auto& item : resourcesSupplied) {    
        stocks[item] = 0;    
    }
}

void Supplier::run() {
    logger() << "Supplier " <<  uniqueId << " starting with fund " << money << std::endl;

    while (true) {
        clock->worker_wait_day_start();
        if (PcoThread::thisThread()->stopRequested()) break;

        attemptToProduceResource();

        clock->worker_end_day();
    }

    logger() << "Supplier " <<  uniqueId << " stopping with fund " << money << std::endl;
}

void Supplier::attemptToProduceResource() {

    // TODO

}

int Supplier::buy(ItemType it, int qty) {

    // TODO
}

void Supplier::pay(int bill) {

    // TODO

}

int Supplier::getMaterialCost() {
    int totalCost = 0;
    for (const auto& item : resourcesSupplied) {
        totalCost += getCostPerUnit(item);
    }
    return totalCost;
}

bool Supplier::sellsResource(ItemType item) const {
    return std::find(resourcesSupplied.begin(), resourcesSupplied.end(), item) != resourcesSupplied.end();
}
