#ifndef CLINIC_H
#define CLINIC_H

#include <vector>
#include <pcosynchro/pcomutex.h>

#include "seller.h"
#include "supplier.h"

/**
 * @brief Represents a Clinic in the healthcare simulation.
 *
 * Inherits from Seller. Handles patient treatment, resource management,
 * billing with suppliers, and interaction with hospitals and insurance.
 */
class Clinic : public Seller
{
public:
    /// Allows test classes to access protected/private members
    friend class TestableClinic;

    /**
     * @brief Constructs a Clinic with a unique ID, initial funds, and required resources.
     * @param uniqueId Unique identifier for the clinic.
     * @param fund Initial capital.
     * @param resourcesNeeded List of ItemType required to treat patients.
     */
    Clinic(int uniqueId, int fund, std::vector<ItemType> resourcesNeeded);


    // Seller interface overrides

    /**
     * @brief Receive a transfer request of Sick Patients from a Hospital.
     * @param what Item type to transfer.
     * @param qty Quantity to transfer.
     * @return Amount of Sick Patients accepted in the transfer.
     */
    int transfer(ItemType what, int qty) override;

    /**
     * @brief Clinics cannot sell items
     */
    int buy(ItemType, int) override {
        throw std::logic_error("Clinic::buy() not supported");
    }

    /**
     * @brief Clinics cannot invoice others
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
     * @brief Sets associated hospitals and resource suppliers for the clinic.
     * @param hospitals Vector of hospital Sellers.
     * @param suppliers Vector of supplier Sellers.
     */
    void setHospitalsAndSuppliers(std::vector<Seller*> hospitals, std::vector<Seller*> suppliers);

    /**
     * @brief Sets the insurance company associated with this clinic.
     * @param insurance Pointer to the Seller representing the insurance.
     */
    void setInsurance(Seller* insurance);


    // Simulation / operational methods

    /**
     * @brief Main loop for the clinic thread.
     *        Handles patient treatment, resource ordering, and sending patients to rehab.
     */
    void run();

    /**
     * @brief Returns the cost of treating one patient.
     */
    int getTreatmentCost();

    /**
     * @brief Returns the number of patients currently being treated.
     */
    int getNumberPatients();

    /**
     * @brief Returns the number of patients waiting for treatment.
     */
    int getWaitingPatients();

private:
    // Internal helper methods

    /**
     * @brief Orders missing resources from suppliers.
     */
    void orderResources();

    /**
     * @brief Checks if clinic has all resources needed to treat one patient.
     */
    [[nodiscard]] bool hasResourcesForTreatment() const;

    /**
     * @brief Chooses a random supplier for a given item.
     * @param item The item type to source.
     * @return Pointer to the selected Supplier.
     */
    Supplier* chooseRandomSupplier(ItemType item);

    /**
     * @brief Pays any unpaid bills to suppliers.
     */
    void payBills();

    /**
     * @brief Processes the next patient if resources and funds allow.
     */
    void processNextPatient();

    /**
     * @brief Sends treated patients to rehabilitation facilities (hospitals).
     */
    void sendPatientsToRehab();


    // Attributes

    std::vector<Seller*> suppliers;               ///< List of resource suppliers
    std::vector<Seller*> hospitals;               ///< Associated hospitals
    Seller* insurance = nullptr;                  ///< Associated insurance

    std::vector<std::pair<Supplier*, int>> unpaidBills; ///< List of unpaid bills to suppliers

    const std::vector<ItemType> resourcesNeeded; ///< Resources required for treatment

    int queueSick = 0;                            ///< Number of patients waiting for treatment

protected:
    /**
     * @brief Treats a single patient.
     */
    virtual void treatOne();
};


// Specialized clinic types

/**
 * @brief Pulmonology Clinic specialization.
 */
class Pulmonology : public Clinic {
public:
    Pulmonology(int uniqueId, int fund);
};

/**
 * @brief Cardiology Clinic specialization.
 */
class Cardiology : public Clinic {
public:
    Cardiology(int uniqueId, int fund);
};

/**
 * @brief Neurology Clinic specialization.
 */
class Neurology : public Clinic {
public:
    Neurology(int uniqueId, int fund);
};

#endif /* CLINIC_H */
