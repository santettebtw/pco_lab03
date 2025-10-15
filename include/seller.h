#ifndef SELLER_H
#define SELLER_H

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <algorithm>

#include <pcosynchro/pcologger.h>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>

#include "costs.h"
#include "day_clock.h"

/**
 * @brief Represents the different types of "items" managed or exchanged by sellers.
 */
enum class ItemType {
    SickPatient,
    RehabPatient,
    Syringe,
    Pill,
    Scalpel,
    Thermometer,
    Stethoscope,
    Nothing
};

/**
 * @brief Represents different types of employees and their workplace context.
 */
enum class EmployeeType {
    Supplier,             ///< Works in supplier shops.
    EmergencyStaff,       ///< Works in ambulances.
    NursingStaff,         ///< Works in hospitals.
    TreatmentSpecialist,  ///< Works in clinics.
    Nothing
};

/**
 * @brief Represents various healthcare service types.
 */
enum class ServiceType {
    Transport,         ///< Ambulance transport to hospital.
    PreTreatmentStay,  ///< Hospital pre-treatment stay.
    Treatment,         ///< Clinic treatment service.
    Rehab              ///< Hospital rehabilitation.
};


// Global helper functions

int getCostPerUnit(ItemType item);
int getCostPerService(ServiceType claim);
std::string getItemName(ItemType item);
EmployeeType getEmployeeThatProduces(ItemType item);
int getEmployeeSalary(EmployeeType employee);

/**
 * @brief Abstract base class representing an economic actor (clinic, supplier, etc.)
 *        involved in the simulation. Handles funds, inventory, and inter-seller exchanges.
 */
class Seller {
public:
    /**
     * @brief Constructs a Seller with initial funds and unique identifier.
     * @param money Initial amount of money available.
     * @param uniqueId Unique identifier for this seller instance.
     */
    Seller(int money, int uniqueId) : money(money), uniqueId(uniqueId) {
        logger().setVerbosity(1);
    }

    virtual ~Seller() = default;


    // Pure virtual interface — must be implemented

    /**
     * @brief Called by the sender when organizing a transfer of items to this Seller instance.
     * @param what Item type being transferred.
     * @param qty Quantity of items the sender asks to transfer.
     * @return Amount of items that this Seller instance accepts in the transfer.
     */
    virtual int transfer(ItemType what, int qty) = 0;

    /**
     * @brief Called by the buyer to purchase items.
     * @param what Item type to buy.
     * @param qty Quantity the buyer wants to buy.
     * @return Amount of items that this Seller instance accepts to sell.
     */
    virtual int buy(ItemType what, int qty) = 0;

    /**
     * @brief Called by the beneficiary to invoice another Seller.
     * @param bill Amount to invoice.
     * @param who The beneficiary of the invoice.
     */
    virtual void invoice(int bill, Seller* who) = 0;

    /**
     * @brief Called by the payer to settle a bill.
     * @param bill Amount to pay.
     */
    virtual void pay(int bill) = 0;


    // Utility functions

    /**
     * @brief Computes the total amount paid to employees of a given type.
     */
    int getAmountPaidToEmployees(EmployeeType employeeType) const {
        return nbEmployeesPaid * getEmployeeSalary(employeeType);
    }

    /**
     * @brief Returns a copy of the current stock.
     */
    std::map<ItemType, int> getStock() const { return stocks; }

    /**
     * @brief Returns the current funds of this Seller.
     */
    [[nodiscard]] int getFund() const { return money; }

    /**
     * @brief Returns this Seller's unique identifier.
     */
    [[nodiscard]] int getUniqueId() const { return uniqueId; }

    /**
     * @brief Selects a random Seller from a list.
     * @param sellers List of available sellers.
     * @return Pointer to the randomly chosen Seller.
     */
    static Seller* chooseRandomSeller(std::vector<Seller*>& sellers);

    /**
     * @brief Selects a random item from a map of available items.
     * @param itemsForSale Map of items and quantities.
     * @return The randomly chosen item type.
     */
    static ItemType chooseRandomItem(std::map<ItemType, int>& itemsForSale);

    /**
     * @brief Selects a random item currently in this Seller's stock.
     * @return The randomly chosen item type.
     */
    ItemType getRandomItemFromStock();

    /**
     * @brief Associates a DayClock with this Seller (for time-based simulation logic).
     */
    void setClock(DayClock* c) { clock = c; }

protected:
    // ─────────────────────────────────────────────
    // Protected attributes
    // ─────────────────────────────────────────────

    std::map<ItemType, int> stocks;  ///< Inventory of available items.
    int money;                       ///< Current amount of funds.
    int uniqueId;                    ///< Unique identifier for this seller.
    int nbEmployeesPaid{0};          ///< Total number of employees paid.
    DayClock* clock{nullptr};        ///< Pointer to the simulation clock.
};

#endif // SELLER_H
