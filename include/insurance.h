#ifndef INSURANCE_H
#define INSURANCE_H

#include <pcosynchro/pcomutex.h>
#include "seller.h"

/**
 * @class Insurance
 * @brief Represents an insurance entity that handles financial transactions within the healthcare system.
 *
 * The Insurance class inherits from Seller and models an insurance company that
 * receives invoices from medical institutions (e.g., hospitals or clinics),
 * processes payments, and collects regular contributions from its clients.
 */
class Insurance : public Seller {
public:
    friend class TestableInsurance;

    /**
     * @brief Constructs an Insurance instance.
     * @param uniqueId Unique identifier of the insurance entity.
     * @param fund Initial capital or financial reserve.
     */
    Insurance(int uniqueId, int fund);

    /**
     * @brief Disabled: Insurance cannot sell resources.
     * @throws std::logic_error Always, since Insurance::buy() is unsupported.
     */
    int buy(ItemType, int) override {
        throw std::logic_error("Insurance::buy() not supported");
    }

    /**
     * @brief Disabled: Insurance cannot receive item transfers.
     * @throws std::logic_error Always, since Insurance::transfer() is unsupported.
     */
    int transfer(ItemType, int) override {
        throw std::logic_error("Insurance::transfer() not supported");
    }

    /**
     * @brief Disabled: Insurance can not receive direct payments via this interface.
     * @throws std::logic_error Always, since Insurance::pay() is unsupported.
     */
    void pay(int) override {
        throw std::logic_error("Insurance::pay() not supported");
    }

    /**
     * @brief Receives an invoice from a healthcare provider (e.g., hospital or clinic).
     *
     * The insurance records the bill for later processing. The actual payment
     * is handled asynchronously within its operational loop (`run()`).
     *
     * @param bill Amount of the bill.
     * @param who Pointer to the Seller (hospital or clinic) who issued the invoice.
     */
    void invoice(int bill, Seller* who) override;

    /**
     * @brief Main execution loop of the insurance entity.
     *
     * Simulates the regular behavior of an insurance company by:
     * - Collecting periodic contributions.
     * - Paying pending invoices to healthcare providers.
     */
    void run();

private:
    /**
     * @brief Simulates the reception of periodic insurance contributions.
     *
     * This function increases the available funds, representing
     * the income from insured individuals or institutions.
     */
    void receiveContributions();

    /**
     * @brief Processes and pays pending bills from healthcare providers.
     *
     * Iterates over the list of unpaid invoices, deducts the corresponding
     * amounts from available funds, and sends payments to the respective Sellers.
     */
    void payBills();

private:
    const int INSURANCE_CONTRIBUTION = 1;
    std::vector<std::pair<Seller*, int>> unpaidBills; ///< List of healthcare providers (Sellers) awaiting payment and their corresponding bill amounts.
};

#endif // INSURANCE_H
