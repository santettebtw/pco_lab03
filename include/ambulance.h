#ifndef AMBULANCE_H
#define AMBULANCE_H

#include "seller.h"

/**
 * @brief Represents an Ambulance in the healthcare simulation.
 *
 * Inherits from Seller but overrides most trading functions.
 * Responsible for transporting patients from the field to
 * hospitals and interacting with insurance.
 */
class Ambulance : public Seller {
public:
    /// Allows test classes to access protected/private members
    friend class TestableAmbulance;

    /**
     * @brief Constructs an Ambulance with a unique ID, initial funds,
     *        the types of resources it can supply, and initial stock quantities.
     * @param uniqueId Unique identifier for the ambulance.
     * @param fund Initial funds available to the ambulance.
     * @param resourcesSupplied List of item types that the ambulance can supply.
     * @param initialStocks Map of initial stock quantities for each item type.
     */
    Ambulance(int uniqueId, int fund,
              std::vector<ItemType> resourcesSupplied,
              std::map<ItemType,int> initialStocks);


    // Seller interface overrides

    /**
     * @brief Ambulances cannot sell items
     */
    int buy(ItemType, int) override {
        throw std::logic_error("Ambulance::buy() not supported");
    }

    /**
     * @brief Ambulances do not receive transfers
     */
    int transfer(ItemType, int) override {
        throw std::logic_error("Ambulance::transfer() not supported");
    }

    /**
     * @brief Ambulances cannot invoice others
     */
    void invoice(int /*bill*/, Seller* /*who*/) override {
        throw std::logic_error("Supplier::invoice() not supported");
    }

    /**
     * @brief Receive a bill paiement (e.g., from insurance).
     * @param bill Bill amount.
     */
    void pay(int bill) override;


    // Configuration

    /**
     * @brief Sets the hospitals that this ambulance can deliver patients to.
     * @param hospitals Vector of pointers to hospital Sellers.
     */
    void setHospitals(std::vector<Seller*> hospitals);

    /**
     * @brief Sets the insurance company that the ambulance interacts with.
     * @param insurance Pointer to a Seller representing the insurance.
     */
    void setInsurance(Seller* insurance);


    // Simulation / operational methods

    /**
     * @brief Main loop for the ambulance thread.
     *        Handles patient transportation and interaction with insurance.
     */
    void run();

    /**
     * @brief Returns the current number of patients in the ambulance.
     * @return Number of patients.
     */
    int getNumberPatients();

    /**
     * @brief Returns the list of resources that this ambulance can supply.
     * @return Vector of ItemType values.
     */
    [[nodiscard]] std::vector<ItemType> getResourcesSupplied() const;

protected:
    /**
     * @brief Sends patients to hospitals according to availability and logic.
     *        Called internally by run().
     */
    void sendPatients();


    // Protected attributes

    std::vector<ItemType> resourcesSupplied;  ///< Types of resources the ambulance carries.
    std::vector<Seller*> hospitals;           ///< Hospitals that can receive patients.
    Seller* insurance{nullptr};               ///< Insurance company for billing.
};

#endif // AMBULANCE_H
