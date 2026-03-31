// ─────────────────────────────────────────────────────────────────────────────
// main.cpp – TI-84 CE port of App Manager
// ─────────────────────────────────────────────────────────────────────────────
#include "../include/ce_gui.hpp"
#include "../include/chemistry.hpp"

int main(void) {
    TIGui gui;
    ChemistryApp chemistry;
    chemistry.run(gui);
    return 0;
}
