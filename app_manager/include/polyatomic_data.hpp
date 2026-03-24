#ifndef POLYATOMIC_DATA_HPP
#define POLYATOMIC_DATA_HPP

#include "chemistry.hpp"
#include <vector>

const std::vector<PolyatomicIon> POLYATOMIC_DATA = {
    // Common Polyatomic Ions
    {"Ammonium",                        "NH4+",      +1},
    {"Acetate",                         "CH3COO-",   -1},
    {"Bromate",                         "BrO3-",     -1},
    {"Carbonate",                       "CO3^2-",    -2},
    {"Chlorate",                        "ClO3-",     -1},
    {"Chlorite",                        "ClO2-",     -1},
    {"Chromate",                        "CrO4^2-",   -2},
    {"Cyanide",                         "CN-",       -1},
    {"Dichromate",                      "Cr2O7^2-",  -2},
    {"Hydrogen carbonate / Bicarbonate","HCO3-",     -1},
    {"Hydrogen sulfate / Bisulfate",    "HSO4-",     -1},
    {"Hydrogen sulfite / Bisulfite",    "HSO3-",     -1},
    {"Hydrogen phosphate / Biphosphate","HPO4^2-",   -2},
    {"Hydroxide",                       "OH-",       -1},
    {"Hypochlorite",                    "ClO-",      -1},
    {"Iodate",                          "IO3-",      -1},
    {"Nitrate",                         "NO3-",      -1},
    {"Nitrite",                         "NO2-",      -1},
    {"Oxalate",                         "C2O4^2-",   -2},
    {"Perchlorate",                     "ClO4-",     -1},
    {"Periodate",                       "IO4-",      -1},
    {"Permanganate",                    "MnO4-",     -1},
    {"Peroxide",                        "O2^2-",     -2},
    {"Phosphate",                       "PO4^3-",    -3},
    {"Phosphite",                       "PO3^3-",    -3},
    {"Silicate",                        "SiO4^4-",   -4},
    {"Sulfate",                         "SO4^2-",    -2},
    {"Sulfite",                         "SO3^2-",    -2},
    {"Thiocyanate",                     "SCN-",      -1},
    {"Thiosulfate",                     "S2O3^2-",   -2},
    // Other Ions
    {"Copper (I) / Cuprous",            "Cu+",       +1},
    {"Copper (II) / Cupric",            "Cu2+",      +2},
    {"Iron (II) / Ferrous",             "Fe2+",      +2},
    {"Iron (III) / Ferric",             "Fe3+",      +3},
    {"Lead (II) / Plumbous",            "Pb2+",      +2},
    {"Lead (IV) / Plumbic",             "Pb4+",      +4},
    {"Mercury (I) / Mercurous",         "Hg2^2+",    +2},
    {"Mercury (II) / Mercuric",         "Hg2+",      +2},
    {"Tin (II) / Stannous",             "Sn2+",      +2},
    {"Tin (IV) / Stannic",              "Sn4+",      +4},
};

#endif // POLYATOMIC_DATA_HPP
