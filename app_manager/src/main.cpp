#include <os.h>
#include "../include/ti_gui.hpp"
#include "../include/app.hpp"
#include "../include/gauss_app.hpp"
#include "../include/chemistry.hpp"
#include "../include/scratchpad_app.hpp"
#include "../include/inverse_app.hpp"
#include <vector>

extern "C" void _fini() {}
extern "C" void _init() {}

int main(void) {
    TIGui gui;
    std::vector<App*> apps;
    
    GaussApp gauss;
    ChemistryApp chemistry;
    InverseApp inverse;
    //ScratchpadApp scratchpad;
    
    apps.push_back(&gauss);
    apps.push_back(&chemistry);
    apps.push_back(&inverse);
    //apps.push_back(&scratchpad);

    int selected_app = 0;
    bool running = true;

    while (running) {
        gui.clear(COLOR_WHITE);
        gui.drawTopBar("App Manager");

        gui.drawText(20, 30, "Select an App:", COLOR_DARK_GRAY);

        for (size_t i = 0; i < apps.size(); i++) {
            int y = 60 + i * 30;
            if (static_cast<int>(i) == selected_app) {
                gui.fillRect(15, y - 2, 290, 24, COLOR_BLUE);
                gui.drawText(20, y, apps[i]->getName(), COLOR_WHITE);
            } else {
                gui.drawText(20, y, apps[i]->getName(), COLOR_BLACK);
            }
        }

        gui.drawBottomBar(" ^/v: Select | ENTER: Launch | ESC: Exit ");
        gui.render();

        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                int sym = event.key.keysym.sym;
                if (sym == SDLK_ESCAPE) {
                    running = false;
                } else if (sym == SDLK_UP || sym == SDLK_KP8) {
                    if (selected_app > 0) selected_app--;
                } else if (sym == SDLK_DOWN || sym == SDLK_KP2) {
                    if (selected_app < static_cast<int>(apps.size()) - 1) selected_app++;
                } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                    apps[selected_app]->run(gui);
                }
            }
        }
    }

    return 0;
}
