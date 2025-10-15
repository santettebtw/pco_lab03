#include "seller.h"
#include <random>
#include <cassert>

Seller *Seller::chooseRandomSeller(std::vector<Seller *> &sellers) {
    assert(sellers.size());
    std::vector<Seller*> out;
    std::sample(sellers.begin(), sellers.end(), std::back_inserter(out),
            1, std::mt19937{std::random_device{}()});
    return out.front();
}

ItemType Seller::chooseRandomItem(std::map<ItemType, int> &itemsForSale) {
    if (!itemsForSale.size()) {
        return ItemType::Nothing;
    }
    std::vector<std::pair<ItemType, int> > out;
    std::sample(itemsForSale.begin(), itemsForSale.end(), std::back_inserter(out),
            1, std::mt19937{std::random_device{}()});
    return out.front().first;
}

ItemType Seller::getRandomItemFromStock() {
    if (stocks.empty()) {
        throw std::runtime_error("Stock is empty.");
    }

    auto it = stocks.begin();
    std::advance(it, rand() % stocks.size());

    return it->first;
}

int getCostPerUnit(ItemType item) {
    switch (item) {
        case ItemType::Syringe : return SYRINGUE_COST;
        case ItemType::Pill : return PILL_COST;
        case ItemType::Scalpel : return SCALPEL_COST;
        case ItemType::Thermometer : return THERMOMETER_COST;
        case ItemType::Stethoscope : return STETHOSCOPE_COST;
        default : return 0;
    }
}

std::string getItemName(ItemType item) {
    switch (item) {
        case ItemType::Syringe : return "Syringe";
        case ItemType::Pill : return "Pill";
        case ItemType::Scalpel : return "Scalpel";
        case ItemType::Thermometer : return "Thermometer";
        case ItemType::Stethoscope : return "Stethoscope";
        case ItemType::RehabPatient : return "Rehab Patient";
        case ItemType::SickPatient : return "Sick Patient";
        case ItemType::Nothing : return "Nothing";
        default : return "???";
    }
}

EmployeeType getEmployeeThatProduces(ItemType item) {
    switch (item) {
        case ItemType::Syringe : /* fallthrough */
        case ItemType::Pill : /* fallthrough */
        case ItemType::Scalpel : /* fallthrough */
        case ItemType::Thermometer : /* fallthrough */
        case ItemType::SickPatient : /* fallthrough */
        case ItemType::Stethoscope : return EmployeeType::Supplier;
        case ItemType::RehabPatient : return EmployeeType::TreatmentSpecialist;
        default : return EmployeeType::Nothing;
    }
}

int getEmployeeSalary(EmployeeType employee) {
    switch (employee) {
        case EmployeeType::EmergencyStaff : return NURSE_COST;
        case EmployeeType::NursingStaff : return NURSE_COST;
        case EmployeeType::Supplier : return SUPPLIER_COST;
        case EmployeeType::TreatmentSpecialist : return DOCTOR_COST;
        default : return 0;
    }
}

int getCostPerService(ServiceType claim) {
    switch (claim) {
        case ServiceType::Transport : return TRANSFER_COST;
        case ServiceType::PreTreatmentStay : return PRETREATMENT_COST;
        case ServiceType::Treatment : return TREATMENT_COST;
        case ServiceType::Rehab : return REHAB_COST;
        default : return 0;
    }
}
