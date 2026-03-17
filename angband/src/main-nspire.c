/**
 * \file main-nspire.c
 * \brief Main file for playing Angband on the TI-Nspire CX II (Ndless)
 *
 * This file provides the Angband terminal (term) back-end for the
 * TI-Nspire CX II calculator running Ndless.  It replaces the ncurses
 * front-end that is used on UNIX systems.
 *
 * Architecture overview
 * ---------------------
 *  - nspire/nspire-draw.{c,h}  : framebuffer + bitmap font rendering
 *  - nspire/nspire-event.{c,h} : ring-buffer event queue
 *  - nspire/nspire-input.{c,h} : hardware keyboard polling + key mapping
 *  - this file                 : Angband "term" hooks + main()
 *
 * Terminal dimensions
 * -------------------
 *  Screen  : 320 × 240 px (RGB-565)
 *  Font    : 4 × 8 px (4-column slice of the embedded 5×8 bitmap font)
 *  Term    : 80 columns × 30 rows
 *
 * File system layout on the calculator
 * --------------------------------------
 *  /documents/angband/lib/      – Angband data directory
 *  /documents/angband/lib/save/ – save files
 *
 * Copyright (c) 2026 – Ndless/Angband contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *   a) the GNU General Public License as published by the Free Software
 *      Foundation, version 2, or
 *   b) the "Angband licence" (see copying.txt).
 */

#ifdef USE_NSPIRE

#include <libndls.h>
#include <os.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "angband.h"
#include "buildid.h"
#include "init.h"
#include "main.h"
#include "savefile.h"
#include "ui-display.h"
#include "ui-game.h"
#include "ui-input.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "ui-init.h"
#include "z-color.h"

/* Nspire-specific headers */
#include "nspire/nspire-draw.h"
#include "nspire/nspire-event.h"
#include "nspire/nspire-input.h"

/* -----------------------------------------------------------------------
 * term_data – one instance per Angband window (we only use one)
 * -------------------------------------------------------------------- */
typedef struct term_data {
    term t;
} term_data;

#define MAX_TERM_DATA 1

static term_data s_term_data[MAX_TERM_DATA];

/* -----------------------------------------------------------------------
 * Pre-computed RGB-565 colour table (indexed by Angband COLOUR_* values)
 * -------------------------------------------------------------------- */
static nspire_pixel_t s_colour_table[MAX_COLORS];

static void nspire_init_colours(void)
{
    for (int i = 0; i < MAX_COLORS; i++) {
        /* angband_color_table[i] = { gamma, R, G, B }  (0–255 each) */
        uint8_t r = angband_color_table[i][1];
        uint8_t g = angband_color_table[i][2];
        uint8_t b = angband_color_table[i][3];
        s_colour_table[i] = nspire_rgb(r, g, b);
    }
}

/* -----------------------------------------------------------------------
 * Term hook: Term_init_nspire
 * -------------------------------------------------------------------- */
static void Term_init_nspire(term *t)
{
    (void)t;
    /* Nothing to do; the display is already initialised in main(). */
}

/* -----------------------------------------------------------------------
 * Term hook: Term_nuke_nspire
 * -------------------------------------------------------------------- */
static void Term_nuke_nspire(term *t)
{
    (void)t;
    /* Nothing to do. */
}

/* -----------------------------------------------------------------------
 * Term hook: Term_xtra_nspire  (handles all TERM_XTRA_* requests)
 * -------------------------------------------------------------------- */

/* Poll / wait for a single event.  Returns 0 if an event was consumed,
 * non-zero if none was available (and wait==false).                    */
static int check_events(bool wait)
{
    do {
        /* Scan hardware keyboard → fills the event queue */
        nspire_input_scan();

        if (nspire_event_ready()) {
            nspire_event_t ev = nspire_event_get();

            if (ev.type == NSPIRE_EVENT_KEYBOARD) {
                Term_keypress(ev.keyboard.key, ev.keyboard.mods);
            }
            return 0;
        }

        /* Nothing ready – if not waiting, bail out */
        if (!wait)
            return 1;

        /* Small sleep to avoid burning CPU while waiting */
        msleep(10);

    } while (wait);

    return 1;
}

static errr Term_xtra_nspire(int n, int v)
{
    (void)v;

    switch (n) {

    case TERM_XTRA_EVENT:
        return check_events((bool)v);

    case TERM_XTRA_FLUSH:
        /* Drain all pending events */
        while (!check_events(false))
            ;
        return 0;

    case TERM_XTRA_CLEAR:
        nspire_clear(s_colour_table[COLOUR_DARK]);
        return 0;

    case TERM_XTRA_SHAPE:
        /* Cursor visibility – nothing to do */
        return 0;

    case TERM_XTRA_FROSH:
        /* Row finished; actual LCD blit is deferred to TERM_XTRA_FRESH so
         * we only call the expensive lcd_blit() once per full frame instead
         * of 30 times (once per row). */
        return 0;

    case TERM_XTRA_FRESH:
        /* Full redraw finished – push framebuffer to LCD */
        nspire_video_flush();
        return 0;

    case TERM_XTRA_NOISE:
        /* Beep – not supported */
        return 0;

    case TERM_XTRA_BORED:
        /* Nothing interesting happening; handle events non-blocking */
        return check_events(false);

    case TERM_XTRA_REACT:
        /* Global settings changed (e.g. colour table) */
        nspire_init_colours();
        return 0;

    case TERM_XTRA_ALIVE:
        /* Suspend / resume – nothing to do */
        return 0;

    case TERM_XTRA_LEVEL:
        /* Window activated / deactivated – nothing to do */
        return 0;

    case TERM_XTRA_DELAY:
        /* Delay v milliseconds */
        if (v > 0)
            msleep((unsigned)v);
        return 0;
    }

    return 1;  /* Unknown / unhandled action */
}

/* -----------------------------------------------------------------------
 * Term hook: Term_curs_nspire  (draw the cursor)
 * -------------------------------------------------------------------- */
static errr Term_curs_nspire(int x, int y)
{
    nspire_draw_cursor(x, y);
    return 0;
}

/* -----------------------------------------------------------------------
 * Term hook: Term_wipe_nspire  (erase n cells starting at x,y)
 * -------------------------------------------------------------------- */
static errr Term_wipe_nspire(int x, int y, int n)
{
    nspire_pixel_t dark = s_colour_table[COLOUR_DARK];
    for (int i = 0; i < n; i++)
        nspire_draw_char(x + i, y, ' ', dark, dark);
    return 0;
}

/* -----------------------------------------------------------------------
 * Term hook: Term_text_nspire  (draw n characters starting at x,y)
 * -------------------------------------------------------------------- */
static errr Term_text_nspire(int x, int y, int n, int a, const wchar_t *s)
{
    nspire_pixel_t fg = s_colour_table[a % MAX_COLORS];
    nspire_pixel_t bg;

    switch (a / MULT_BG) {
    case BG_SAME:
        bg = fg;
        break;
    case BG_DARK:
        bg = s_colour_table[COLOUR_SHADE];
        break;
    case BG_BLACK:
    default:
        bg = s_colour_table[COLOUR_DARK];
        break;
    }

    for (int i = 0; i < n; i++) {
        /* Truncate to 8-bit ASCII for our bitmap font.
         * Wide chars outside 0x00–0xFF are rendered as '?'. */
        int ch = (int)(s[i]);
        if (ch < 0 || ch > 0xFF) ch = '?';
        nspire_draw_char(x + i, y, ch, fg, bg);
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * term_data_link – set up one Angband term window
 * -------------------------------------------------------------------- */
static void term_data_link(int idx)
{
    term_data *td = &s_term_data[idx];
    term       *t = &td->t;

    term_init(t, NSPIRE_TERM_COLS, NSPIRE_TERM_ROWS, 256);

    /* Use a soft cursor drawn explicitly by Term_curs_nspire */
    t->soft_cursor = true;

    /* We never receive TERM_XTRA_BORED spam (we handle it ourselves) */
    t->never_bored = true;

    /* Hook up the callbacks */
    t->init_hook = Term_init_nspire;
    t->nuke_hook = Term_nuke_nspire;
    t->xtra_hook = Term_xtra_nspire;
    t->curs_hook = Term_curs_nspire;
    t->wipe_hook = Term_wipe_nspire;
    t->text_hook = Term_text_nspire;

    t->data = (void *)td;

    Term_activate(t);
}

/* -----------------------------------------------------------------------
 * init_nspire – initialise terminal, colours, and Angband globals
 * -------------------------------------------------------------------- */
errr init_nspire(void)
{
    nspire_init_colours();

    memset(s_term_data, 0, sizeof(s_term_data));

    /* Create windows in reverse order so window 0 is activated last */
    for (int i = MAX_TERM_DATA - 1; i >= 0; i--) {
        term_data_link(i);
        angband_term[i] = Term;
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * File paths
 * -------------------------------------------------------------------- */
static void init_files(void)
{
    char root[256];

    /* get_documents_dir() returns the virtual root (e.g. "/documents/") */
    const char *docs = get_documents_dir();
    snprintf(root, sizeof(root), "%sangband/lib/", docs);

    init_file_paths(root, root, root);

    /* Default save file */
    char save[256];
    snprintf(save, sizeof(save), "%sangband/lib/save/PLAYER", docs);
    my_strcpy(savefile, save, sizeof(savefile));

    create_needed_dirs();
}

/* -----------------------------------------------------------------------
 * Logging hooks (called by Angband's plog / quit internals)
 * -------------------------------------------------------------------- */
static void hook_plog(const char *str)
{
    if (str)
        nspire_log(str);
}

static void hook_quit(const char *str)
{
    if (str) {
        nspire_log(str);
        /* Keep screen visible so the user can read the error */
        wait_key_pressed();
    }
}

/* -----------------------------------------------------------------------
 * main()
 * -------------------------------------------------------------------- */
int main(void)
{
    /* Make sure we're running on a colour model */
    if (!has_colors) {
        /* Classic Nspire – show a message in the OS dialog and exit */
        show_msgbox("Angband",
                    "Angband requires a TI-Nspire CX (colour screen).");
        return 1;
    }

    /* Bump CPU speed for better performance */
    set_cpu_speed(CPU_SPEED_150MHZ);

    /* Initialise display */
    nspire_video_init();
    nspire_clear(NSPIRE_BLACK);
    nspire_video_flush();

    /* Initialise event queue */
    if (!nspire_event_init()) {
        nspire_log("Failed to initialise event queue.\n");
        wait_key_pressed();
        return 1;
    }

    /* Install Angband hooks */
    plog_aux  = hook_plog;
    quit_aux  = hook_quit;

    /* Initialise Angband terminal */
    if (init_nspire()) {
        nspire_log("No terminals initialised!\n");
        wait_key_pressed();
        return 1;
    }

    ANGBAND_SYS = "nspire";

    /* Set up data file paths */
    init_files();

    /* Set command hook */
    cmd_get_hook = textui_get_cmd;

    /* Initialise and run the game */
    init_display();
    init_angband();
    textui_init();

    /* Wait for player to press a key before starting */
    pause_line(Term);

    /* Main game loop */
    play_game(GAME_LOAD);

    /* Clean up */
    textui_cleanup();
    cleanup_angband();

    quit(NULL);
    return 0;
}

#endif /* USE_NSPIRE */
