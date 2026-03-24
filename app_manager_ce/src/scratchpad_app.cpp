// ─────────────────────────────────────────────────────────────────────────────
// scratchpad_app.cpp – TI-84 CE port
//
// The Nspire version used nl_lua_getstate() to inject CAS "Define" commands
// into the OS Lua runtime.  That mechanism does not exist on the TI-84 CE.
//
// CE replacement: "Python Formula Reference"
//   Displays Python-equivalent function definitions the user can copy into
//   a .py program file on the Python Edition.  Scrollable list, ESC to return.
// ─────────────────────────────────────────────────────────────────────────────
#include "../include/scratchpad_app.hpp"
#include "../include/ce_gui.hpp"
#include <cstdio>

// Each entry shown as two lines: a Python def and a brief description.
struct PyFormula {
    const char* defn;   // Python one-liner
    const char* desc;   // short description
};

static const PyFormula FORMULAS[] = {
    { "def disc(a,b,c): return b**2-4*a*c",      "Discriminant" },
    { "def quad1(a,b,c):",                         "Quadratic (+)" },
    { "  return(-b+(b**2-4*a*c)**.5)/(2*a)",       "" },
    { "def quad2(a,b,c):",                         "Quadratic (-)" },
    { "  return(-b-(b**2-4*a*c)**.5)/(2*a)",       "" },
    { "def dist2d(x1,y1,x2,y2):",                 "2-D Distance" },
    { "  return((x2-x1)**2+(y2-y1)**2)**.5",       "" },
    { "def slope(x1,y1,x2,y2):",                  "Slope" },
    { "  return(y2-y1)/(x2-x1)",                   "" },
    { "def ke(m,v): return 0.5*m*v**2",            "Kinetic energy" },
    { "def pe(m,g,h): return m*g*h",               "Potential energy" },
    { "def fnet(m,a): return m*a",                 "Newton 2nd law" },
};
static const int NUM_FORMULAS = (int)(sizeof(FORMULAS) / sizeof(FORMULAS[0]));

static int scroll = 0;

void ScratchpadApp::run(TIGui& gui) {
    scroll = 0;
    bool running = true;
    const int VISIBLE = 8;  // rows visible at a time

    while (running) {
        gui.clear(COLOR_WHITE);
        gui.drawTopBar("Python Formula Reference");
        gui.drawText(5, 23, "Copy these into your .py program:", COLOR_DARK_GRAY);

        for (int i = 0; i < VISIBLE && (scroll + i) < NUM_FORMULAS; i++) {
            int idx = scroll + i;
            int y = 36 + i * 22;
            gui.fillRect(5, y - 1, 310, 20, COLOR_LIGHT_GRAY);

            char buf[48];
            snprintf(buf, sizeof(buf), "%s", FORMULAS[idx].defn);
            gui.drawText(8, y + 2, buf, COLOR_BLACK);

            if (FORMULAS[idx].desc[0]) {
                // right-align the description label
                gui.drawText(230, y + 2, FORMULAS[idx].desc, COLOR_DARK_GRAY);
            }
        }

        // Scroll indicators
        if (scroll > 0)
            gui.drawText(306, 36, "^", COLOR_DARK_GRAY);
        if (scroll + VISIBLE < NUM_FORMULAS)
            gui.drawText(306, 36 + (VISIBLE - 1) * 22, "v", COLOR_DARK_GRAY);

        char count_buf[32];
        snprintf(count_buf, sizeof(count_buf), "%d formulas  (CE: copy only)",
                 NUM_FORMULAS);
        gui.drawText(5, 214, count_buf, COLOR_DARK_GRAY);

        gui.drawBottomBar(" ^/v: Scroll | CLR: Back ");
        gui.render();

        int key = gui.waitKey();
        if (key == KEY_ESCAPE) {
            running = false;
        } else if (key == KEY_UP && scroll > 0) {
            scroll--;
        } else if (key == KEY_DOWN && scroll + VISIBLE < NUM_FORMULAS) {
            scroll++;
        }
    }
}
