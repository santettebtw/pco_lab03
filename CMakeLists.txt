cmake_minimum_required(VERSION 3.13)
project(PCO_LAB03)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-g)

# ---------- Core (lib) ----------
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/supplier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clinic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/seller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hospital.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ambulance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/insurance.cpp
)
set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/supplier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/clinic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/seller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hospital.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ambulance.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/costs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/insurance.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/day_clock.h
)

add_library(hospital_core ${CORE_SOURCES} ${HEADERS})


target_include_directories(hospital_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(hospital_core PUBLIC pcosynchro)


add_executable(pco_hospital ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(pco_hospital PRIVATE hospital_core)

# ---------- Tests ----------
enable_testing()
find_package(GTest REQUIRED)

add_executable(unit_tests
   tests/test_ambulance.cpp
   tests/test_clinic.cpp
   tests/test_hospital.cpp
   tests/test_insurance.cpp
   tests/test_supplier.cpp
)

target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(unit_tests PRIVATE hospital_core GTest::gtest GTest::gtest_main)

add_test(NAME unit_tests COMMAND unit_tests)
