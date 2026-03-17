#ifndef TI_GUI_HPP
#define TI_GUI_HPP

#include <os.h>
#include <SDL/SDL_config.h>
#include <SDL/SDL.h>
#include <string>
#include <vector>

using namespace std;

// Colors
enum TIGuiColor {
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_LIGHT_GRAY,
    COLOR_DARK_GRAY,
    COLOR_BLUE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_ORANGE,
    COLOR_PURPLE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_PINK,
    COLOR_BROWN,
    COLOR_TEAL
};

class TIGui {
private:
    SDL_Surface* screen;
    nSDL_Font* font_black;
    nSDL_Font* font_white;
    nSDL_Font* font_gray;
    nSDL_Font* font_blue;
    nSDL_Font* font_red;

    Uint32 mapColor(TIGuiColor color) {
        switch(color) {
            case COLOR_BLACK: return SDL_MapRGB(screen->format, 0, 0, 0);
            case COLOR_WHITE: return SDL_MapRGB(screen->format, 255, 255, 255);
            case COLOR_LIGHT_GRAY: return SDL_MapRGB(screen->format, 220, 220, 220);
            case COLOR_DARK_GRAY: return SDL_MapRGB(screen->format, 100, 100, 100);
            case COLOR_BLUE: return SDL_MapRGB(screen->format, 0, 120, 215);
            case COLOR_RED: return SDL_MapRGB(screen->format, 200, 0, 0);
            case COLOR_GREEN: return SDL_MapRGB(screen->format, 34, 139, 34);
            case COLOR_ORANGE: return SDL_MapRGB(screen->format, 255, 140, 0);
            case COLOR_PURPLE: return SDL_MapRGB(screen->format, 128, 0, 128);
            case COLOR_YELLOW: return SDL_MapRGB(screen->format, 255, 215, 0);
            case COLOR_CYAN: return SDL_MapRGB(screen->format, 0, 255, 255);
            case COLOR_MAGENTA: return SDL_MapRGB(screen->format, 255, 0, 255);
            case COLOR_PINK: return SDL_MapRGB(screen->format, 255, 105, 180);
            case COLOR_BROWN: return SDL_MapRGB(screen->format, 139, 69, 19);
            case COLOR_TEAL: return SDL_MapRGB(screen->format, 0, 128, 128);
            default: return SDL_MapRGB(screen->format, 0, 0, 0);
        }
    }

    nSDL_Font* getFont(TIGuiColor color) {
        switch(color) {
            case COLOR_BLACK: return font_black;
            case COLOR_WHITE: return font_white;
            case COLOR_DARK_GRAY:
            case COLOR_LIGHT_GRAY: return font_gray;
            case COLOR_BLUE: return font_blue;
            case COLOR_RED: return font_red;
            // Map the rest to black or colored fonts if we added them (let's map to black to save memory limits, except we need colored fonts if we want colored text. For now background fill is colored, text is black/white)
            default: return font_black;
        }
    }

public:
    TIGui() {
        SDL_Init(SDL_INIT_VIDEO);
        screen = SDL_SetVideoMode(320, 240, has_colors ? 16 : 8, SDL_SWSURFACE);
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        
        font_black = nSDL_LoadFont(NSDL_FONT_THIN, 0, 0, 0);
        font_white = nSDL_LoadFont(NSDL_FONT_THIN, 255, 255, 255);
        font_gray  = nSDL_LoadFont(NSDL_FONT_THIN, 100, 100, 100);
        font_blue  = nSDL_LoadFont(NSDL_FONT_THIN, 0, 120, 215);
        font_red   = nSDL_LoadFont(NSDL_FONT_THIN, 200, 0, 0);
    }

    ~TIGui() {
        nSDL_FreeFont(font_black);
        nSDL_FreeFont(font_white);
        nSDL_FreeFont(font_gray);
        nSDL_FreeFont(font_blue);
        nSDL_FreeFont(font_red);
        SDL_Quit();
    }

    void clear(TIGuiColor color = COLOR_WHITE) {
        SDL_FillRect(screen, NULL, mapColor(color));
    }

    void drawRect(int x, int y, int w, int h, TIGuiColor color) {
        SDL_Rect top = {(Sint16)x, (Sint16)y, (Uint16)w, 1};
        SDL_Rect bottom = {(Sint16)x, (Sint16)(y + h - 1), (Uint16)w, 1};
        SDL_Rect left = {(Sint16)x, (Sint16)y, 1, (Uint16)h};
        SDL_Rect right = {(Sint16)(x + w - 1), (Sint16)y, 1, (Uint16)h};
        
        Uint32 c = mapColor(color);
        SDL_FillRect(screen, &top, c);
        SDL_FillRect(screen, &bottom, c);
        SDL_FillRect(screen, &left, c);
        SDL_FillRect(screen, &right, c);
    }

    void fillRect(int x, int y, int w, int h, TIGuiColor color) {
        SDL_Rect r = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
        SDL_FillRect(screen, &r, mapColor(color));
    }

    void fillRectRGB(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
        SDL_Rect rect = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
        SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, r, g, b));
    }

    void drawText(int x, int y, const string& text, TIGuiColor color = COLOR_BLACK) {
        nSDL_DrawString(screen, getFont(color), x, y, text.c_str());
    }

    void drawTopBar(const string& title) {
        fillRect(0, 0, 320, 20, COLOR_BLUE);
        drawText(5, 5, title, COLOR_WHITE);
    }
    void drawBottomBar(const string& text) {
        fillRect(0, 225, 320, 15, COLOR_LIGHT_GRAY);
        drawText(5, 227, text, COLOR_BLACK);
    }

    void render() {
        SDL_Flip(screen);
    }
    bool handleTextInput(int sym, string& str, int max_len = 8, bool allow_negative = true, bool allow_decimal = true) {
        if (sym == SDLK_BACKSPACE || sym == SDLK_DELETE) {
            if (!str.empty()) str.pop_back();
            return true;
        }
        
        char ch = 0;
        if (sym >= SDLK_0 && sym <= SDLK_9) ch = '0' + (sym - SDLK_0);
        else if (sym >= SDLK_KP0 && sym <= SDLK_KP9) ch = '0' + (sym - SDLK_KP0);
        else if (allow_negative && (sym == SDLK_MINUS || sym == SDLK_KP_MINUS)) ch = '-';
        else if (allow_decimal && (sym == SDLK_PERIOD || sym == SDLK_KP_PERIOD)) ch = '.';
        
        if (ch == '-' && str.find('-') != string::npos) ch = 0;
        if (ch == '.' && str.find('.') != string::npos) ch = 0;
        if (ch == '-' && str.length() > 0) ch = 0;
        
        if (ch != 0 && str.length() < (size_t)max_len) {
            str += ch;
            return true;
        }
        return false;
    }
};

#endif
