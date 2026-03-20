/**
 * \file nspire/nspire-draw.h
 * \brief Display driver for the TI-Nspire CX II (Ndless) — BrogueCE port
 *
 * Draws directly into a 320×240 RGB-565 software framebuffer that is
 * pushed to the LCD via lcd_blit() when nspire_video_flush() is called.
 *
 * Font cell: 4×8 pixels (derived from the NDS 5×8 font, columns 0-3,
 * rows 0-7).
 *   Terminal columns : 320 / 4 = 80 exactly  → uses 80 (COLS)
 *   Terminal rows    : 240 / 8 = 30 exactly  → uses 30 (ROWS)
 *
 * Copyright (c) 2026 — Ndless/BrogueCE contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, version 3 or later.
 */

#ifndef NSPIRE_DRAW_H
#define NSPIRE_DRAW_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Screen geometry
 * -------------------------------------------------------------------- */
#define NSPIRE_SCREEN_W      320
#define NSPIRE_SCREEN_H      240

/* Font cell size.
 * We use the full 4×8 left portion of the 5×8 NDS bitmap font
 * (columns 0-3, rows 0-7).  This fits BrogueCE's COLS=80, ROWS=30
 * terminal exactly within the 320×240 screen (80×4=320, 30×8=240). */
#define NSPIRE_FONT_W        4
#define NSPIRE_FONT_H        8

/* No horizontal offset needed: 80 cols × 4 px = 320 px exactly.   */
#define NSPIRE_X_OFFSET      0

#define NSPIRE_TERM_COLS     80    /* == COLS from Rogue.h          */
#define NSPIRE_TERM_ROWS     30    /* == ROWS from Rogue.h (27+3)   */

/* -----------------------------------------------------------------------
 * Pixel type – RGB-565 stored in a 16-bit word
 * -------------------------------------------------------------------- */
typedef uint16_t nspire_pixel_t;

/** Pack 8-bit R/G/B into RGB-565. */
static inline nspire_pixel_t nspire_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (nspire_pixel_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/** Pack 0-100 R/G/B (BrogueCE scale) into RGB-565. */
static inline nspire_pixel_t nspire_rgb_100(short r, short g, short b)
{
    /* Scale 0-100 → 0-255 with a simple multiply then shift */
    return nspire_rgb((uint8_t)((r * 255) / 100),
                      (uint8_t)((g * 255) / 100),
                      (uint8_t)((b * 255) / 100));
}

#define NSPIRE_WHITE   nspire_rgb(0xFF, 0xFF, 0xFF)
#define NSPIRE_BLACK   nspire_rgb(0x00, 0x00, 0x00)

/* -----------------------------------------------------------------------
 * Public interface
 * -------------------------------------------------------------------- */

/** Initialise the framebuffer and call lcd_init(). */
void nspire_video_init(void);

/**
 * Flush dirty rows of the software framebuffer to the physical LCD.
 * Call before blocking for input so the player sees the latest display.
 */
void nspire_video_flush(void);

/** Fill the entire framebuffer with the given pixel value. */
void nspire_clear(nspire_pixel_t colour);

/**
 * Draw one character glyph at terminal cell (col, row).
 * @param col  0-based terminal column  (0 … NSPIRE_TERM_COLS-1)
 * @param row  0-based terminal row     (0 … NSPIRE_TERM_ROWS-1)
 * @param c    ASCII character to render (values outside 0x20-0x7E are blank)
 * @param fg   foreground colour (RGB-565)
 * @param bg   background colour (RGB-565)
 */
void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg);

/**
 * Draw a debug/log string starting at pixel position (px, py).
 * Primarily for boot-time error messages.
 */
void nspire_log(const char *msg);

#endif /* NSPIRE_DRAW_H */
