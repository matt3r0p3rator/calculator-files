/**
 * \file nspire-platform.c
 * \brief BrogueCE platform back-end for the TI-Nspire CX II (Ndless)
 *
 * Implements the `brogueConsole` struct that BrogueCE uses to abstract
 * all platform-dependent I/O.  Replaces the SDL2 / ncurses front-ends.
 *
 * Architecture
 * ------------
 *  nspire/nspire-draw.{c,h}  : 320×240 RGB-565 framebuffer + 3×7 font
 *  nspire/nspire-event.{c,h} : ring-buffer rogueEvent queue
 *  nspire/nspire-input.{c,h} : hardware keyboard polling + key mapping
 *  this file                 : brogueConsole hooks + glyphToAscii helper
 *
 * Screen layout
 * -------------
 *  Screen : 320 × 240 px (RGB-565)
 *  Font   : 3 × 7 px (cols 1-3, rows 1-7 of the embedded NDS 5×8 font)
 *  Term   : 100 columns × 34 rows  (== BrogueCE COLS × ROWS)
 *
 * File system layout on the calculator
 * --------------------------------------
 *  /documents/brogue.tns       ← the executable
 *  /documents/brogue/          ← working directory (save files, scores)
 *
 * Copyright (c) 2026 – Ndless/BrogueCE contributors
 * License: GNU Affero General Public License v3 or later
 */

#ifdef BROGUE_NSPIRE

#include <libndls.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "Rogue.h"
#include "GlobalsBase.h"
#include "Globals.h"

#include "nspire/nspire-draw.h"
#include "nspire/nspire-event.h"
#include "nspire/nspire-input.h"

/* -----------------------------------------------------------------------
 * Glyph → ASCII helper (mirrors curses-platform.c glyphToAscii)
 * -------------------------------------------------------------------- */

static char nspire_glyph_to_ascii(enum displayGlyph glyph)
{
    /* All glyph values below 128 are plain ASCII. */
    unsigned int ch = glyphToUnicode(glyph);
    if (ch < 0x80)
        return (char)ch;

    /* Map non-ASCII glyphs to visually representative ASCII fallbacks.
     * These match the mappings used by curses-platform.c.              */
    switch (glyph) {
    case G_UP_ARROW:         return '^';
    case G_DOWN_ARROW:       return 'v';
    case G_FLOOR:            return '.';
    case G_FLOOR_ALT:        return '.';
    case G_CHASM:            return ':';
    case G_TRAP:             return '%';
    case G_FIRE:             return '^';
    case G_FOLIAGE:          return '&';
    case G_BLOODWORT_STALK:  return '&';
    case G_AMULET:           return ',';
    case G_SCROLL:           return '?';
    case G_RING:             return '=';
    case G_WEAPON:           return '(';
    case G_GEM:              return '+';
    case G_TOTEM:            return '0';
    case G_GOOD_MAGIC:       return '$';
    case G_BAD_MAGIC:        return '+';
    case G_DOORWAY:          return '<';
    case G_CHARM:            return '7';
    case G_GUARDIAN:         return '5';
    case G_WINGED_GUARDIAN:  return '5';
    case G_EGG:              return 'o';
    case G_UNICORN:          return 'U';
    case G_TURRET:           return '*';
    case G_CARPET:           return '.';
    case G_STATUE:           return '5';
    case G_CRACKED_STATUE:   return '5';
    case G_MAGIC_GLYPH:      return ':';
    case G_ELECTRIC_CRYSTAL: return '$';
    default:                 return '?';
    }
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: gameLoop
 * -------------------------------------------------------------------- */
static void nspire_gameLoop(void)
{
    rogueMain();
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: pauseForMilliseconds
 * -------------------------------------------------------------------- */
static boolean nspire_pauseForMilliseconds(short milliseconds,
                                           PauseBehavior behavior)
{
    /* Flush display so the player sees the current frame. */
    nspire_video_flush();

    int remaining = (int)milliseconds;
    while (remaining > 0) {
        nspire_input_scan();
        if (nspire_event_ready())
            return true;

        int chunk = (remaining < SCAN_INTERVAL_MS) ? remaining : SCAN_INTERVAL_MS;
        msleep((unsigned)chunk);
        remaining -= chunk;
    }

    /* One final scan after the delay expires. */
    nspire_input_scan();
    return nspire_event_ready();
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: nextKeyOrMouseEvent
 * -------------------------------------------------------------------- */
static void nspire_nextKeyOrMouseEvent(rogueEvent *returnEvent,
                                       boolean textInput,
                                       boolean colorsDance)
{
    nspire_video_flush();

    for (;;) {
        if (colorsDance) {
            shuffleTerrainColors(3, true);
            commitDraws();
            nspire_video_flush();
        }

        nspire_input_scan();

        if (nspire_event_ready()) {
            *returnEvent = nspire_event_get();
            return;
        }

        msleep(SCAN_INTERVAL_MS);
    }
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: plotChar
 * -------------------------------------------------------------------- */
static void nspire_plotChar(enum displayGlyph inputChar,
                            short xLoc, short yLoc,
                            short foreRed, short foreGreen, short foreBlue,
                            short backRed, short backGreen, short backBlue)
{
    /* Map glyph to a printable ASCII character. */
    char c = nspire_glyph_to_ascii(inputChar);
    if (c < 0x20) c = ' ';

    nspire_pixel_t fg = nspire_rgb_100(foreRed, foreGreen, foreBlue);
    nspire_pixel_t bg = nspire_rgb_100(backRed, backGreen, backBlue);

    nspire_draw_char((int)xLoc, (int)yLoc, (unsigned char)c, fg, bg);
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: remap  (no-op — key mapping done in input.c)
 * -------------------------------------------------------------------- */
static void nspire_remap(const char *input_name, const char *output_name)
{
    (void)input_name;
    (void)output_name;
    /* The Nspire key layout is fixed in nspire-input.c; keymaps are not
     * used.  loadKeymap() opens keymap.txt which won't exist on the
     * calculator, so this callback is never meaningfully called.        */
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: modifierHeld
 *   modifier 0 → Shift key
 *   modifier 1 → Ctrl key
 * -------------------------------------------------------------------- */
static boolean nspire_modifierHeld(int modifier)
{
    if (modifier == 0) return isKeyPressed(KEY_NSPIRE_SHIFT) ? true : false;
    if (modifier == 1) return isKeyPressed(KEY_NSPIRE_CTRL)  ? true : false;
    return false;
}

/* -----------------------------------------------------------------------
 * brogueConsole callback: setGraphicsMode (always text-only)
 * -------------------------------------------------------------------- */
static enum graphicsModes nspire_setGraphicsMode(enum graphicsModes mode)
{
    (void)mode;
    return TEXT_GRAPHICS;
}

/* -----------------------------------------------------------------------
 * The console struct (exported so main-nspire.c can assign it)
 * -------------------------------------------------------------------- */
struct brogueConsole nspireConsole = {
    nspire_gameLoop,
    nspire_pauseForMilliseconds,
    nspire_nextKeyOrMouseEvent,
    nspire_plotChar,
    nspire_remap,
    nspire_modifierHeld,
    NULL,                   /* notifyEvent: not required */
    NULL,                   /* takeScreenshot: not supported */
    nspire_setGraphicsMode
};

#endif /* BROGUE_NSPIRE */
