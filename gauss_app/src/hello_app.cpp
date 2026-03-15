#include "../include/hello_app.hpp"
#include <os.h>

void HelloApp::run(TIGui& gui) {
    bool running = true;
    while(running) {
        gui.clear(COLOR_WHITE);
        gui.drawTopBar("Hello World App");
        
        gui.drawText(60, 100, "Hello from the new App!", COLOR_BLUE);
        gui.drawText(60, 140, "Press ESC to return.", COLOR_DARK_GRAY);
        
        gui.drawBottomBar(" ESC: Exit ");
        gui.render();
        
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }
    }
}