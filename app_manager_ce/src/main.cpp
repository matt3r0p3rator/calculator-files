// ─────────────────────────────────────────────────────────────────────────────
// main.cpp – TI-84 CE port of App Manager
// ─────────────────────────────────────────────────────────────────────────────
#include "../include/ce_gui.hpp"
#include "../include/app.hpp"
#include "../include/gauss_app.hpp"
#include "../include/chemistry.hpp"
#include "../include/scratchpad_app.hpp"
#include "../include/inverse_app.hpp"
#include <stdio.h>

int main(void) {
    TIGui gui;

    GaussApp      gauss;
    ChemistryApp  chemistry;
    InverseApp    inverse;
    ScratchpadApp formulas;

    App* apps[] = { &chemistry, &gauss, &inverse, &formulas };
    const int NUM_APPS = 4;

    int  selected_app = 0;
    bool running      = true;

    while (running) {
        gui.clear(COLOR_WHITE);
        gui.drawTopBar("App Manager  CE");

        gui.drawText(20, 30, "Select an App:", COLOR_DARK_GRAY);

        for (int i = 0; i < NUM_APPS; i++) {
            int y = 60 + i * 30;
            if (i == selected_app) {
                gui.fillRect(15, y - 2, 290, 24, COLOR_BLUE);
                gui.drawText(20, y, apps[i]->getName(), COLOR_WHITE);
            } else {
                gui.drawText(20, y, apps[i]->getName(), COLOR_BLACK);
            }
        }

        gui.drawBottomBar(" ^/v:Select | ENTER:Launch | CLR:Exit ");
        gui.render();

        int key = gui.waitKey();
        if (key == KEY_ESCAPE) {
            running = false;
        } else if (key == KEY_UP) {
            if (selected_app > 0) selected_app--;
        } else if (key == KEY_DOWN) {
            if (selected_app < NUM_APPS - 1) selected_app++;
        } else if (key == KEY_ENTER) {
            apps[selected_app]->run(gui);
        }
    }

    return 0;
}
