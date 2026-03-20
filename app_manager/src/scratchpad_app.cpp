#include "../include/scratchpad_app.hpp"
#include "../include/ti_gui.hpp"
#include <nucleus.h>
#include <string>
#include <cstdio>

using namespace std;

// --- Define your functions here ---
// Each entry is a TI-Nspire CAS "Define" statement.
static const char* USER_DEFINES[] = {
    "Define disc(a,b,c)=b^2-4*a*c",
    "Define quad1(a,b,c)=(-b+sqrt(b^2-4*a*c))/(2*a)",
    "Define quad2(a,b,c)=(-b-sqrt(b^2-4*a*c))/(2*a)",
    "Define dist2d(x1,y1,x2,y2)=sqrt((x2-x1)^2+(y2-y1)^2)",
    "Define slope(x1,y1,x2,y2)=(y2-y1)/(x2-x1)",
    "Define ke(m,v)=0.5*m*v^2",
    "Define pe(m,g,h)=m*g*h",
    "Define fnet(m,a)=m*a",
};
static const int NUM_DEFINES = sizeof(USER_DEFINES) / sizeof(USER_DEFINES[0]);

// Scroll offset for the preview list
static int scroll = 0;

void ScratchpadApp::run(TIGui& gui) {
    scroll = 0;
    bool running = true;
    const int VISIBLE = 6; // rows visible at once

    while (running) {
        gui.clear(COLOR_WHITE);
        gui.drawTopBar("Init Scratchpad Functions");
        gui.drawText(10, 23, "Functions to define:", COLOR_DARK_GRAY);

        for (int i = 0; i < VISIBLE && (scroll + i) < NUM_DEFINES; i++) {
            int y = 38 + i * 22;
            gui.fillRect(10, y - 1, 300, 20, COLOR_LIGHT_GRAY);
            // Truncate long strings to fit the screen (~38 chars)
            char buf[40];
            snprintf(buf, sizeof(buf), "%s", USER_DEFINES[scroll + i]);
            gui.drawText(13, y + 2, buf, COLOR_BLACK);
        }

        // Scroll indicators
        if (scroll > 0)
            gui.drawText(305, 38, "^", COLOR_DARK_GRAY);
        if (scroll + VISIBLE < NUM_DEFINES)
            gui.drawText(305, 38 + (VISIBLE - 1) * 22, "v", COLOR_DARK_GRAY);

        char count[32];
        snprintf(count, sizeof(count), "Total: %d functions", NUM_DEFINES);
        gui.drawText(10, 175, count, COLOR_DARK_GRAY);

        gui.drawBottomBar(" ^/v: Scroll | ENTER: Initialize | ESC: Back ");
        gui.render();

        SDL_Event event;
        if (!SDL_WaitEvent(&event)) continue;
        if (event.type != SDL_KEYDOWN) continue;
        int sym = event.key.keysym.sym;

        if (sym == SDLK_ESCAPE) {
            running = false;
        } else if ((sym == SDLK_UP || sym == SDLK_KP8) && scroll > 0) {
            scroll--;
        } else if ((sym == SDLK_DOWN || sym == SDLK_KP2) && scroll + VISIBLE < NUM_DEFINES) {
            scroll++;
        } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
            // Use the OS Lua state to call math.eval() for each Define statement
            lua_State *L = nl_lua_getstate();
            int ok = 0;
            if (L) {
                for (int d = 0; d < NUM_DEFINES; d++) {
                    char lua_cmd[160];
                    snprintf(lua_cmd, sizeof(lua_cmd), "math.eval(\"%s\")", USER_DEFINES[d]);
                    if (luaL_dostring(L, lua_cmd) == 0) ok++;
                }
            }

            // Result screen
            bool done = false;
            while (!done) {
                gui.clear(COLOR_WHITE);
                gui.drawTopBar("Init Scratchpad Functions");
                gui.fillRect(30, 70, 260, 70, L ? COLOR_GREEN : COLOR_RED);
                if (L) {
                    gui.drawText(50, 85, "Functions initialized!", COLOR_WHITE);
                    char msg[40];
                    snprintf(msg, sizeof(msg), "%d / %d defined OK", ok, NUM_DEFINES);
                    gui.drawText(70, 110, msg, COLOR_WHITE);
                } else {
                    gui.drawText(45, 85, "Lua state unavailable.", COLOR_WHITE);
                    gui.drawText(40, 110, "Run from scratchpad instead.", COLOR_WHITE);
                }
                gui.drawBottomBar(" Press any key to return ");
                gui.render();
                SDL_Event ev;
                if (SDL_WaitEvent(&ev) && ev.type == SDL_KEYDOWN)
                    done = true;
            }
        }
    }
}
