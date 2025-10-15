#ifndef HOSPITAL_H
#define HOSPITAL_H

#include <vector>
#include <pcosynchro/pcomutex.h>
#include "seller.h"

/**
 * @class Hospital
 * @brief Represents a hospital that receives and treats patients coming from ambulances or clinics.
 *
 * The Hospital class inherits from Seller and models a healthcare unit capable of admitting
 * patients (both sick and in rehabilitation), managing available beds, and coordinating with
 * clinics and insurance entities. It also handles the internal economic logic such as paying staff.
 */
class Hospital : public Seller {
public:
    friend class TestableHospital;

    /**
     * @brief Constructs a Hospital instance.
     * @param uniqueId Unique identifier of the hospital.
     * @param fund Initial budget or available funds.
     * @param maxBeds Maximum number of beds available in the hospital.
     */
    Hospital(int uniqueId, int fund, int maxBeds);

    /**
     * @brief Receives a transfer request of patients (sick or in rehabilitation)
     *        from an Ambulance or a Clinic.
     * @param what Type of item (typically a patient type).
     * @param qty Quantity of patients to transfer.
     * @return Number of patients accepted by the hospital.
     */
    int transfer(ItemType what, int qty) override;

    /**
     * @brief Hospital do not sell resources.
     * @throws std::logic_error Always, since Hospital::buy() is unsupported.
     */
    int buy(ItemType, int) override {
        throw std::logic_error("Hospital::buy() not supported");
    }

    /**
     * @brief Hospitals do not issue invoices to others
     * @throws std::logic_error Always, since Hospital::invoice() is unsupported.
     */
    void invoice(int /*bill*/, Seller* /*who*/) override {
        throw std::logic_error("Supplier::invoice() not supported");
    }

    /**
     * @brief Receives a payment (e.g., from an insurance company).
     * @param bill Amount of the payment.
     */
    void pay(int bill) override;

    /**
     * @brief Defines the list of associated clinics to which the hospital
     *        can transfer patients for further treatment or rehabilitation.
     * @param clinics List of pointers to connected clinics.
     */
    void setClinics(std::vector<Seller*> clinics);

    /**
     * @brief Sets the insurance entity responsible for hospital reimbursements.
     * @param insurance Pointer to the insurance Seller.
     */
    void setInsurance(Seller* insurance);

    /**
     * @brief Gets the current number of patients admitted to the hospital.
     * @return Number of occupied beds (patients currently hospitalized).
     */
    int getNumberPatients();

    /**
     * @brief Main operational routine for the hospital.
     *
     * This method simulates the hospital's continuous functioning, including
     * updating patient rehabilitation, transferring patients to clinics, and
     * managing ongoing payments such as nursing staff salaries.
     */
    void run();

private:
    /**
     * @brief Transfers recovered or stable patients to associated clinics.
     */
    void transferSickPatientsToClinic();

    /**
     * @brief Updates rehabilitation status of patients, decrementing remaining days.
     *        Frees beds when rehabilitation is complete.
     */
    void updateRehab();

    /**
     * @brief Handles continuous operational costs such as paying nursing staff.
     */
    void payNursingStaff();

private:
    std::vector<Seller*> clinics;  ///< Clinics associated with this hospital.
    Seller* insurance = nullptr;   ///< Linked insurance provider.

    int maxBeds;                   ///< Maximum number of patients the hospital can accommodate.
    int nbNursingStaff;            ///< Number of nursing staff employed.
    int nbFreed = 0;               ///< Number of patients who have completed treatment and left the hospital.
};

#endif // HOSPITAL_H
