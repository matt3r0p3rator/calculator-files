#ifndef CE_GUI_HPP
#define CE_GUI_HPP

// ─────────────────────────────────────────────────────────────────────────────
// ce_gui.hpp  — TI-84 CE port of ti_gui.hpp
//
// Replaces nSDL / SDL 1.2 with CE GraphX + keypadc.h.
// Screen:  320 × 240 (identical to TI-Nspire — no layout changes needed).
// Colors:  8 bpp palette mode; standard UI colors occupy indices 0–14.
//          Palette index 255 is reserved as the text-background transparent
//          sentinel so all text is rendered with a transparent background.
// Input:   kb_Scan() edge-detection loop replaces SDL_WaitEvent.
//          2nd key  = Tab  |  Mode key   = Menu  |  Clear     = ESC/Back
//          (-)  key = '-'  |  Del  key   = Backspace           (in text input)
//          Alpha  key  toggles alpha-lock for A–Z letter entry
//
// NOTE on alpha-mode letter mapping:
//   The mapping below matches the letters PRINTED ABOVE each key face on the
//   physical TI-84 CE.  If any are wrong for your specific revision, adjust
//   the alphaKeyToKey() block in mapCurrentKey() below.
// ─────────────────────────────────────────────────────────────────────────────

#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <stdint.h>

// ─── Palette indices ──────────────────────────────────────────────────────────
// Keep as uint8_t so they can be passed directly to gfx_SetColor / gfx_SetTextFGColor.
enum TIGuiColor : uint8_t {
    COLOR_BLACK      = 0,
    COLOR_WHITE      = 1,
    COLOR_LIGHT_GRAY = 2,
    COLOR_DARK_GRAY  = 3,
    COLOR_BLUE       = 4,
    COLOR_RED        = 5,
    COLOR_GREEN      = 6,
    COLOR_ORANGE     = 7,
    COLOR_PURPLE     = 8,
    COLOR_YELLOW     = 9,
    COLOR_CYAN       = 10,
    COLOR_MAGENTA    = 11,
    COLOR_PINK       = 12,
    COLOR_BROWN      = 13,
    COLOR_TEAL       = 14,
    // 255 is reserved as transparent sentinel — never used as a visible color
};

// ─── Key codes ────────────────────────────────────────────────────────────────
// These replace SDLK_* throughout the application source files.
enum CEKey {
    KEY_NONE = 0,
    // Navigation
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    // Actions
    KEY_ENTER, KEY_ESCAPE, KEY_TAB,   // TAB = 2nd key, ESC = Clear
    KEY_DEL,                           // Del (backspace in text input)
    KEY_MENU,                          // Mode key (opens colour menu, etc.)
    // Numeric / symbol input (normal mode)
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_PERIOD,   // decimal point  [ . ]
    KEY_MINUS,    // change-sign    [ (-) ]   ← distinct from subtract key
    // Alpha-lock letters (A–Z)
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
    KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
};

// ─────────────────────────────────────────────────────────────────────────────
class TIGui {

    // (Palette data initialised in the constructor body — see below)

    bool alpha_locked = false;   // toggled by pressing the Alpha key

    // ── Helper: is any key currently held? ───────────────────────────────────
    static bool anyKeyHeld() {
        for (uint8_t g = 1; g <= 7; g++)
            if (kb_Data[g]) return true;
        return false;
    }

    // ── Map current kb_Data snapshot → a single CEKey ────────────────────────
    // Called after kb_Scan().  Returns KEY_NONE if no mapped key is found.
    // Alpha mode letters follow immediately after normal mode.
    int mapCurrentKey() {
        // ── Navigation (always active) ─────────────────────────────────────
        if (kb_Data[7] & kb_Up)    return KEY_UP;
        if (kb_Data[7] & kb_Down)  return KEY_DOWN;
        if (kb_Data[7] & kb_Left)  return KEY_LEFT;
        if (kb_Data[7] & kb_Right) return KEY_RIGHT;

        // ── Control keys ──────────────────────────────────────────────────
        if (kb_Data[6] & kb_Enter) return KEY_ENTER;
        if (kb_Data[6] & kb_Clear) return KEY_ESCAPE;
        if (kb_Data[1] & kb_Del)   return KEY_DEL;
        if (kb_Data[1] & kb_2nd)   return KEY_TAB;    // 2nd = Tab
        if (kb_Data[1] & kb_Mode)  return KEY_MENU;

        // ── Alpha key toggles alpha-lock (consumed, not forwarded) ──────
        if (kb_Data[2] & kb_Alpha) {
            alpha_locked = !alpha_locked;
            return KEY_NONE;
        }

        if (alpha_locked) {
            // ── Alpha-mode letter mapping ──────────────────────────────────
            // Letters are printed ABOVE each key face on your physical CE.
            // Layout (verify against your unit if anything seems off):
            //   [MATH]=A  [APPS]=B  [PRGM]=C  [VARS]=D
            //   [^]  =E   [÷]  =F   [×]  =G   [−] =H   [+]=I
            //   [9]  =J   [8]  =K   [7]  =L
            //   [6]  =M   [5]  =N   [4]  =O
            //   [3]  =P   [2]  =Q   [1]  =R
            //   [(-)] =S  [.] =T   [0]  =U
            //   [SIN]=V   [COS]=W  [TAN]=X
            //   [Ln] =Y   [Log]=Z
            if (kb_Data[2] & kb_Math)   return KEY_A;  // [Math]
            if (kb_Data[3] & kb_Apps)   return KEY_B;  // [Apps / i]
            if (kb_Data[4] & kb_Prgm)   return KEY_C;  // [Prgm]
            if (kb_Data[5] & kb_Vars)   return KEY_D;  // [Vars]
            if (kb_Data[6] & kb_Power)  return KEY_E;  // [^]
            if (kb_Data[6] & kb_Div)    return KEY_F;  // [÷]
            if (kb_Data[6] & kb_Mul)    return KEY_G;  // [×]
            if (kb_Data[6] & kb_Sub)    return KEY_H;  // [−]
            if (kb_Data[6] & kb_Add)    return KEY_I;  // [+]
            if (kb_Data[5] & kb_9)      return KEY_J;
            if (kb_Data[4] & kb_8)      return KEY_K;
            if (kb_Data[3] & kb_7)      return KEY_L;
            if (kb_Data[5] & kb_6)      return KEY_M;
            if (kb_Data[4] & kb_5)      return KEY_N;
            if (kb_Data[3] & kb_4)      return KEY_O;
            if (kb_Data[5] & kb_3)      return KEY_P;
            if (kb_Data[4] & kb_2)      return KEY_Q;
            if (kb_Data[3] & kb_1)      return KEY_R;
            if (kb_Data[5] & kb_Chs)    return KEY_S;  // [(-)]
            if (kb_Data[4] & kb_DecPnt) return KEY_T;  // [.]
            if (kb_Data[3] & kb_0)      return KEY_U;
            if (kb_Data[3] & kb_Sin)    return KEY_V;
            if (kb_Data[4] & kb_Cos)    return KEY_W;
            if (kb_Data[5] & kb_Tan)    return KEY_X;
            if (kb_Data[2] & kb_Ln)     return KEY_Y;
            if (kb_Data[2] & kb_Log)    return KEY_Z;
        } else {
            // ── Normal mode: numeric / symbol input ────────────────────────
            if (kb_Data[3] & kb_0)      return KEY_0;
            if (kb_Data[3] & kb_1)      return KEY_1;
            if (kb_Data[4] & kb_2)      return KEY_2;
            if (kb_Data[5] & kb_3)      return KEY_3;
            if (kb_Data[3] & kb_4)      return KEY_4;
            if (kb_Data[4] & kb_5)      return KEY_5;
            if (kb_Data[5] & kb_6)      return KEY_6;
            if (kb_Data[3] & kb_7)      return KEY_7;
            if (kb_Data[4] & kb_8)      return KEY_8;
            if (kb_Data[5] & kb_9)      return KEY_9;
            if (kb_Data[4] & kb_DecPnt) return KEY_PERIOD;
            if (kb_Data[5] & kb_Chs)    return KEY_MINUS;  // change-sign key → '−'
        }
        return KEY_NONE;
    }

public:
    // ── Constructor: initialise GraphX and set up palette ────────────────────
    TIGui() {
        gfx_Begin();
        // Install our 15 UI colours into palette slots 0–14.
        // Declared as a local array so gfx_RGBTo1555 works regardless of
        // whether the toolchain defines it as a macro or an inline function.
        uint16_t palette[15] = {
            gfx_RGBTo1555(  0,   0,   0),  //  0  BLACK
            gfx_RGBTo1555(255, 255, 255),  //  1  WHITE
            gfx_RGBTo1555(220, 220, 220),  //  2  LIGHT_GRAY
            gfx_RGBTo1555(100, 100, 100),  //  3  DARK_GRAY
            gfx_RGBTo1555(  0, 120, 215),  //  4  BLUE
            gfx_RGBTo1555(200,   0,   0),  //  5  RED
            gfx_RGBTo1555( 34, 139,  34),  //  6  GREEN
            gfx_RGBTo1555(255, 140,   0),  //  7  ORANGE
            gfx_RGBTo1555(128,   0, 128),  //  8  PURPLE
            gfx_RGBTo1555(255, 215,   0),  //  9  YELLOW
            gfx_RGBTo1555(  0, 255, 255),  // 10  CYAN
            gfx_RGBTo1555(255,   0, 255),  // 11  MAGENTA
            gfx_RGBTo1555(255, 105, 180),  // 12  PINK
            gfx_RGBTo1555(139,  69,  19),  // 13  BROWN
            gfx_RGBTo1555(  0, 128, 128),  // 14  TEAL
        };
        gfx_SetPalette(palette, sizeof(palette), 0);
        // Use palette index 255 as transparent sentinel for text background;
        // this makes every gfx_PrintStringXY call render with a transparent
        // background, so text floats cleanly over coloured rectangles.
        gfx_SetTextBGColor(255);
        gfx_SetTextTransparentColor(255);
        // Enable double-buffering: draw to off-screen buffer, flip with render()
        gfx_SetDrawBuffer();
    }

    ~TIGui() {
        gfx_End();
    }

    // ── Drawing primitives ────────────────────────────────────────────────────
    void clear(TIGuiColor color = COLOR_WHITE) {
        gfx_FillScreen((uint8_t)color);
    }

    void drawRect(int x, int y, int w, int h, TIGuiColor color) {
        gfx_SetColor((uint8_t)color);
        gfx_Rectangle(x, y, w, h);
    }

    void fillRect(int x, int y, int w, int h, TIGuiColor color) {
        gfx_SetColor((uint8_t)color);
        gfx_FillRectangle(x, y, w, h);
    }

    // Used by the chemistry heatmap.  Maps computed smooth-gradient RGB values
    // back to the nearest of our five standard spectrum colours so we don't
    // require per-frame palette manipulation.
    void fillRectRGB(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        uint8_t idx;
        if      (r > 200 && g <  80 && b <  80) idx = COLOR_RED;
        else if (r > 150 && g > 150 && b <  80) idx = COLOR_YELLOW;
        else if (r <  80 && g > 150 && b <  80) idx = COLOR_GREEN;
        else if (r <  80 && g > 150 && b > 150) idx = COLOR_CYAN;
        else if (r <  80 && g <  80 && b > 150) idx = COLOR_BLUE;
        else                                      idx = COLOR_LIGHT_GRAY;
        gfx_SetColor(idx);
        gfx_FillRectangle(x, y, w, h);
    }

    void drawText(int x, int y, const char* text,
                  TIGuiColor color = COLOR_BLACK) {
        gfx_SetTextFGColor((uint8_t)color);
        gfx_PrintStringXY(text, x, y);
    }

    void drawTopBar(const char* title) {
        fillRect(0, 0, 320, 20, COLOR_BLUE);
        drawText(5, 5, title, COLOR_WHITE);
    }

    void drawBottomBar(const char* text) {
        fillRect(0, 225, 320, 15, COLOR_LIGHT_GRAY);
        drawText(5, 227, text, COLOR_BLACK);
    }

    // Flip off-screen buffer to screen and prepare next frame's draw buffer.
    void render() {
        gfx_SwapDraw();
        gfx_SetDrawBuffer();
    }

    // ── Input ─────────────────────────────────────────────────────────────────
    // Blocking: waits for all keys to be released, then waits for a new press.
    // Returns the corresponding CEKey value (KEY_NONE if unmapped).
    int waitKey() {
        // Release phase – wait until no keys are held
        do { kb_Scan(); } while (anyKeyHeld());
        // Press phase – wait until at least one key is down
        int key = KEY_NONE;
        while (key == KEY_NONE) {
            kb_Scan();
            if (anyKeyHeld())
                key = mapCurrentKey();
        }
        return key;
    }

    bool isAlphaMode() const { return alpha_locked; }

    // ── Text input helper ─────────────────────────────────────────────────────
    // Appends a digit / decimal / minus to `str` buffer (must hold max_len+1).
    // Mirrors the original SDL handleTextInput() semantics exactly.
    bool handleTextInput(int key, char* str, int max_len = 8,
                         bool allow_negative = true,
                         bool allow_decimal  = true) {
        int len = (int)strlen(str);
        if (key == KEY_DEL) {
            if (len > 0) { str[len - 1] = '\0'; return true; }
            return false;
        }
        char ch = 0;
        if (key >= KEY_0 && key <= KEY_9)
            ch = '0' + (char)(key - KEY_0);
        else if (allow_negative && key == KEY_MINUS)
            ch = '-';
        else if (allow_decimal && key == KEY_PERIOD)
            ch = '.';

        // Validation: only one '-' allowed, only at start; only one '.'
        if (ch == '-' && strchr(str, '-')) ch = 0;
        if (ch == '.' && strchr(str, '.')) ch = 0;
        if (ch == '-' && len > 0) ch = 0;

        if (ch != 0 && len < max_len) {
            str[len]     = ch;
            str[len + 1] = '\0';
            return true;
        }
        return false;
    }
};

#endif // CE_GUI_HPP
