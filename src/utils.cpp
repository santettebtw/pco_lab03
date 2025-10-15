#include "utils.h"

void endService(const std::vector<std::unique_ptr<PcoThread> > &threads) {
    std::cout << "It's time to end !" << std::endl;
	for (const auto &t : threads) t->requestStop();
}

std::vector<Ambulance*> createAmbulances(int nbAmbulances, int idStart){
    if (nbAmbulances < 1){
        std::cout << "Cannot make the programm work with less than 1 Supplier";
        exit(-1);
    }

    std::vector<Ambulance*> ambulances;

    for(int i = 0; i < nbAmbulances; ++i){
        switch(i % 3) {

            case 0:{
                std::map<ItemType, int> initialAmbulanceStock = {{ItemType::SickPatient, INITIAL_PATIENT_SICK}};
                std::vector<ItemType> patients = {ItemType::SickPatient};

                ambulances.push_back(new Ambulance(
                    i + idStart,
                    SUPPLIER_FUND,
                    patients,
                    initialAmbulanceStock
                ));
                break;
            }
        }
    }
    return ambulances;
}

std::vector<Supplier*> createSuppliers(int nbSuppliers, int idStart) {
    if (nbSuppliers < 1){
        std::cout << "Cannot make the programm work with less than 1 Supplier";
        exit(-1);
    }

    std::vector<Supplier*> suppliers;

    for(int i = 0; i < nbSuppliers; ++i){
        switch(i % 2) {
            case 0:{
                suppliers.push_back(new MedicalDeviceSupplier(i + idStart, SUPPLIER_FUND));
                break;
            }
            case 1:{
                suppliers.push_back(new Pharmacy(i + idStart, SUPPLIER_FUND));
                break;
            }
        }
    }
    return suppliers;
}

std::vector<Clinic*> createClinics(int nbClinics, int idStart) {
    if (nbClinics < 1){
        std::cout << "Cannot make the programm work with less than 1 Clinic";
        exit(-1);
    }

    std::vector<Clinic*> clinics;

    for(int i = 0; i < nbClinics; ++i) {
        switch(i % 3) {
            case 0:
                clinics.push_back(new Pulmonology(i + idStart, CLINICS_FUND));
                break;

            case 1:
                clinics.push_back(new Cardiology(i + idStart, CLINICS_FUND));
                break;

            case 2:
                clinics.push_back(new Neurology(i + idStart, CLINICS_FUND));
                break;
        }
    }


    return clinics;
}

std::vector<Hospital*> createHospitals(int nbHospital, int idStart) {
    if(nbHospital < 1){
        std::cout << "Cannot launch the programm without any hospitalr";
        exit(-1);
    }
    std::vector<Hospital*> hospitals;

    for(int i = 0; i < nbHospital; ++i){
        hospitals.push_back(new Hospital(i + idStart, HOSPITALS_FUND, MAX_BEDS_PER_HOSTPITAL));
    }

    return hospitals;
}
