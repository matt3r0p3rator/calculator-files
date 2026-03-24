#ifndef CHEMISTRY_HPP
#define CHEMISTRY_HPP

#include "app.hpp"

struct Element {
    int atomic_number;
    const char* symbol;
    const char* name;
    double atomic_weight;
    const char* category;
    const char* group;
    const char* period;
    const char* block;
    const char* electron_configuration;
    double electronegativity;
    const char* density;
    int number_of_neutrons;
    int number_of_protons;
    int number_of_electrons;
    double atomic_radius;
    double first_ionization;
    double melting_point;
    double boiling_point;
    int number_of_isotopes;
    const char* discoverer;
    const char* year;
    double specific_heat;
    int number_of_shells;
    int number_of_valence;
    const char* positive_oxidation_states;
    const char* negative_oxidation_states;
};

struct PeriodicTrend {
    const char* name;
    const char* behavior;
    const char* description;
    const char* data_field;
    const char* why;
};

struct PolyatomicIon {
    const char* name;
    const char* formula;
    int charge;
};

class ChemistryApp : public App {
public:
    const char* getName() const override { return "Chemistry Explorer"; }
    void run(TIGui& gui) override;
};

#endif // CHEMISTRY_HPP
