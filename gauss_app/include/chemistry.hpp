
#ifndef CHEMISTRY_HPP
#define CHEMISTRY_HPP

#include "app.hpp"

#include <vector>
struct Element {
    int atomic_number;
    std::string symbol;
    std::string name;
    double atomic_weight;
    std::string category;
    std::string group;
    std::string period;
    std::string block;
    std::string electron_configuration;
    double electronegativity;
    std::string density;
    int number_of_neutrons;
    int number_of_protons;
    int number_of_electrons;
    double atomic_radius;
    double first_ionization;
    double melting_point;
    double boiling_point;
    int number_of_isotopes;
    std::string discoverer;
    std::string year;
    double specific_heat;
    int number_of_shells;
    int number_of_valence;
    std::string positive_oxidation_states;
    std::string negative_oxidation_states;
};
struct PeriodicTrend {
    std::string name;
    std::string behavior;
    std::string description;
    std::string data_field;
    std::string why;
};

class ChemistryApp : public App {
public:
    std::string getName() const override { return "Chemistry Explorer"; }
    void run(TIGui& gui) override;
private:
    std::vector<Element> elements;
    std::vector<PeriodicTrend> trends;
    void loadData();
    void showElementDetails(const Element& element, TIGui& gui);
    void showTrendDetails(const PeriodicTrend& trend, TIGui& gui);
};

#endif // CHEMISTRY_HPP