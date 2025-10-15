#ifndef SUPPLIER_H
#define SUPPLIER_H

#include <pcosynchro/pcomutex.h>

#include "costs.h"
#include "seller.h"

/**
 * @class Supplier
 * @brief Represents a resource supplier in the healthcare system.
 *
 * The Supplier class provides medical items (e.g., syringes, scalpels, pills) to clinics.
 * It inherits from Seller and models the behavior of a supplier who:
 * - Produces resources over time.
 * - Sells resources upon request from clinics.
 * - Manages its finances and available stock safely in a concurrent environment.
 */
class Supplier : public Seller {
public:
    friend class TestableSupplier;

    /**
     * @brief Constructs a Supplier entity.
     * @param uniqueId Unique identifier of the supplier.
     * @param fund Initial amount of money available to the supplier.
     * @param resourcesSupplied List of ItemTypes that the supplier can produce and sell.
     */
    Supplier(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied);

    /**
     * @brief Disabled: Suppliers do not receive item transfers.
     * @throws std::logic_error Always, since Supplier::transfer() is unsupported.
     */
    int transfer(ItemType, int) override {
        throw std::logic_error("Supplier::transfer() not supported");
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
    int buy(ItemType it, int qty) override;

    /**
     * @brief Disabled: Suppliers do not receive invoices.
     * @throws std::logic_error Always, since Supplier::invoice() is unsupported.
     */
    void invoice(int /*bill*/, Seller* /*who*/) override {
        throw std::logic_error("Supplier::invoice() not supported");
    }

    /**
     * @brief Receives payment for a previously issued bill.
     *
     * Adds the payment amount to the supplier’s available funds.
     * Called by a buyer (clinic) when settling a bill.
     *
     * @param bill The amount of money received.
     */
    void pay(int bill) override;

    /**
     * @brief Main execution loop of the supplier.
     *
     * Simulates the continuous activity of the supplier:
     * - Attempts to produce resources periodically.
     * - Pays employees involved in production.
     * - Updates the stock accordingly.
     */
    void run();

    /**
     * @brief Computes the total material cost produced so far.
     * @return The cumulative cost associated with production.
     */
    int getMaterialCost();

    /**
     * @brief Checks whether the supplier sells a given item type.
     * @param item The item type to verify.
     * @return True if the supplier provides this item, false otherwise.
     */
    [[nodiscard]] bool sellsResource(ItemType item) const;

private:
    /**
     * @brief Attempts to produce a random resource from the supplier’s product list.
     *
     * - Chooses a random item from the available resources.
     * - Checks if there are enough funds to pay the employee producing it.
     * - Produces the item and updates stock accordingly.
     */
    void attemptToProduceResource();

private:
    std::vector<ItemType> resourcesSupplied; ///< List of resource types the supplier can produce.
};


/**
 * @class MedicalDeviceSupplier
 * @brief Specialized supplier producing medical devices (e.g., scalpel, thermometer, stethoscope).
 */
class MedicalDeviceSupplier : public Supplier {
public:
    /**
     * @brief Constructs a MedicalDeviceSupplier with predefined items.
     * @param uniqueId Unique identifier of the supplier.
     * @param fund Initial amount of money.
     */
    MedicalDeviceSupplier(int uniqueId, int fund)
        : Supplier(uniqueId, fund, {ItemType::Scalpel, ItemType::Thermometer, ItemType::Stethoscope}) {
    }
};


/**
 * @class Pharmacy
 * @brief Specialized supplier producing pharmaceutical items (e.g., syringes, pills).
 */
class Pharmacy : public Supplier {
public:
    /**
     * @brief Constructs a Pharmacy with predefined items.
     * @param uniqueId Unique identifier of the supplier.
     * @param fund Initial amount of money.
     */
    Pharmacy(int uniqueId, int fund)
        : Supplier(uniqueId, fund, {ItemType::Syringe, ItemType::Pill}) {
    }
};

#endif // SUPPLIER_H
