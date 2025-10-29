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


/**
     * @brief Attempts to produce a random resource from the supplier’s product list.
     *
     * - Chooses a random item from the available resources.
     * - Checks if there are enough funds to pay the employee producing it.
     * - Produces the item and updates stock accordingly.
     */
void Supplier::attemptToProduceResource() {

    if(money < getAmountPaidToEmployees(EmployeeType::Supplier)){
        return;
    }
    std::map<ItemType, int> tmp;
    for (auto item : resourcesSupplied) {
        tmp[item] = 1;
    }
    ItemType recourcess = chooseRandomItem(tmp);
    stocks[recourcess]++;
    money -= getAmountPaidToEmployees(EmployeeType::Supplier);
    nbEmployeesPaid++;
    // TODO
}

/**
     * @brief Handles a purchase request from another Seller (e.g., clinic).
     *
     * Deducts the purchased items from the supplier’s stock and
     * returns the bill to the buyer.
     *
     * @param it The type of item being purchased.
     * @param qty The quantity requested.
     * @return The total cost of the transaction.
     */
int Supplier::buy(ItemType it, int qty) {

    stocks[it] -= qty;
    return getCostPerUnit(it)*qty;

}

void Supplier::pay(int bill) {
    this->money += bill;
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
