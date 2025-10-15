#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <iostream>
#include <pcosynchro/pcothread.h>
#include <pcosynchro/pcosemaphore.h>
#include <memory>

#include "supplier.h"
#include "clinic.h"
#include "hospital.h"
#include "seller.h"
#include "ambulance.h"

#define SUPPLIER_FUND 200
#define CLINICS_FUND 300
#define HOSPITALS_FUND 1000
#define INSURANCE_FUND 1000

#define INITIAL_SCALPEL 400
#define INITIAL_THERMOMETER 350
#define INITIAL_STETHOSCOPE 600

#define INITIAL_PILL 350
#define INITIAL_SYRINGE 530

#define INITIAL_PATIENT_SICK 900

#define MAX_BEDS_PER_HOSTPITAL 35

/**
 * @brief Function that signifies the threads to stop (end of the simulation)
 * @param Threads to stop
 */
void endService(const std::vector<std::unique_ptr<PcoThread>>& threads);

std::vector<Ambulance*> createAmbulances(int nbAmbulances, int idStart);
std::vector<Supplier*> createSuppliers(int nbSuppliers, int idStart);
std::vector<Clinic*> createClinics(int nbClinics, int idStart);
std::vector<Hospital*> createHospitals(int nbHospitals, int idStart);

#endif // UTILS_H
