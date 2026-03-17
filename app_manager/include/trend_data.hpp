#pragma once
#include <string>
#include <vector>
#include "chemistry.hpp"

const std::vector<PeriodicTrend> TREND_DATA = {
    {
        "Atomic Radius",
        "Decreases across a period, increases down a group.",
        "The distance from the nucleus to the outermost stable electron orbital.",
        "atomic_radius",
        "Across a period, increasing nuclear charge pulls electrons closer. Down a group, new electron shells are added."
    },
    {
        "Electronegativity",
        "Increases across a period, decreases down a group.",
        "A measure of the tendency of an atom to attract a bonding pair of electrons.",
        "electronegativity",
        "Across a period, more protons attract electrons more strongly. Down a group, increased shielding and distance weaken attraction."
    },
    {
        "First Ionization",
        "Increases across a period, decreases down a group.",
        "Energy required to remove the most loosely held electron.",
        "first_ionization",
        "Across a period, higher nuclear charge binds electrons more tightly. Down a group, outer electrons are further and shielded."
    },
    {
        "Melting Point",
        "Peaks in transition metals, lower in s and p blocks.",
        "The temperature at which a solid will melt.",
        "melting_point",
        "Transition metals have many delocalized d-electrons that form strong metallic bonds, raising the melting point."
    },
    {
        "Boiling Point",
        "Peaks in transition metals, lower in s and p blocks.",
        "The temperature at which a liquid will boil.",
        "boiling_point",
        "Strong metallic bonding in transition metals requires more thermal energy to break, raising the boiling point."
    },
    {
        "Specific Heat",
        "Generally decreases with increasing atomic weight.",
        "Heat capacity per unit mass of a substance.",
        "specific_heat",
        "Heavier atoms have fewer atoms per gram, so they need less energy per macroscopic mass to increase in temperature."
    },
    {
        "Atomic Number",
        "Linear increase across periods and groups.",
        "The number of protons in the nucleus of an atom.",
        "atomic_number",
        "Defined by the number of protons, which determines the element's identity and is organized sequentially by construction."
    }
};
