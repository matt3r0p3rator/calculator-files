/**
 * \file nspire-draw.h
 * \brief Display driver for the TI-Nspire CX II (Ndless)
 *
 * Replaces the ncurses/NDS display layer.  Draws directly into a
 * 320×240 RGB-565 software framebuffer that is pushed to the LCD via
 * lcd_blit() after every logical frame.
 *
 * Font cell: 4×8 pixels (first 4 columns of the 5×8 NDS bitmap font).
 *   Terminal columns : 320 / 4 = 80
 *   Terminal rows    : 240 / 8 = 30
 *
 * Copyright (c) 2026 – Ndless/Angband contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *   a) the GNU General Public License as published by the Free Software
 *      Foundation, version 2, or
 *   b) the "Angband licence" (see copying.txt).
 */

#ifndef NSPIRE_DRAW_H
#define NSPIRE_DRAW_H

#include "../h-basic.h"

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Screen geometry
 * -------------------------------------------------------------------- */
#define NSPIRE_SCREEN_W      320
#define NSPIRE_SCREEN_H      240

/* Font cell size.
 * We render the 5×8 NDS bitmap font into 4-pixel-wide cells by ignoring
 * the 5th (almost always empty) column.  This gives exactly 80 terminal
 * columns at 320px screen width.
 * Terminal cells start at pixel 0; all pixel x-positions are multiples
 * of 4, keeping every cell 4-byte aligned for 32-bit framebuffer writes. */
#define NSPIRE_FONT_W        4
#define NSPIRE_FONT_H        8

#define NSPIRE_TERM_COLS     (NSPIRE_SCREEN_W / NSPIRE_FONT_W)   /* 80  */
#define NSPIRE_TERM_ROWS     (NSPIRE_SCREEN_H / NSPIRE_FONT_H)   /* 30  */

/* -----------------------------------------------------------------------
 * Pixel type – RGB-565 stored in a 16-bit word
 * -------------------------------------------------------------------- */
typedef uint16_t nspire_pixel_t;

/** Pack 8-bit R/G/B into RGB-565. */
static inline nspire_pixel_t nspire_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (nspire_pixel_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

#define NSPIRE_WHITE   nspire_rgb(0xFF, 0xFF, 0xFF)
#define NSPIRE_BLACK   nspire_rgb(0x00, 0x00, 0x00)
#define NSPIRE_CURSOR  nspire_rgb(0xFF, 0xFF, 0x00)   /* yellow cursor */

/* -----------------------------------------------------------------------
 * Public interface
 * -------------------------------------------------------------------- */

/** Initialise the framebuffer and call lcd_init(). */
void nspire_video_init(void);

/**
 * Flush the software framebuffer to the physical LCD.
 * Call once per logical frame / after a batch of drawing calls.
 */
void nspire_video_flush(void);

/** Fill the entire framebuffer with the given pixel value. */
void nspire_clear(nspire_pixel_t colour);

/**
 * Draw one character glyph at terminal cell (col, row).
 * @param col  0-based terminal column  (0 … NSPIRE_TERM_COLS-1)
 * @param row  0-based terminal row     (0 … NSPIRE_TERM_ROWS-1)
 * @param c    ASCII character to render (values outside 0x20–0x7E are blank)
 * @param fg   foreground colour (RGB-565)
 * @param bg   background colour (RGB-565)
 */
void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg);

/**
 * Draw n characters from wide-char string s at terminal cell (col, row),
 * all in the same fg/bg colours.  Faster than n individual nspire_draw_char()
 * calls because the row-base pointer and pair[] lookup table are computed
 * once for the entire span, and clipping is only checked for the last glyph.
 * @param col  starting terminal column
 * @param row  terminal row
 * @param n    number of characters to draw
 * @param fg   foreground colour (RGB-565)
 * @param bg   background colour (RGB-565)
 * @param s    wide-char string of length >= n
 */
void nspire_draw_chars(int col, int row, int n,
                       nspire_pixel_t fg, nspire_pixel_t bg,
                       const wchar_t *s);

/**
 * Draw a cursor indicator (small rectangle outline) at cell (col, row).
 */
void nspire_draw_cursor(int col, int row);

/**
 * Fill ncols terminal cells on the given row with a solid colour.
 * Faster than calling nspire_draw_char with a space ncols times.
 * @param col    starting terminal column
 * @param row    terminal row
 * @param ncols  number of cells to fill
 * @param colour fill colour (RGB-565)
 */
void nspire_fill_cells(int col, int row, int ncols, nspire_pixel_t colour);

/**
 * Draw a debug/log string starting at pixel position (px, py).
 * Primarily for boot-time error messages.
 */
void nspire_log(const char *msg);
void nspire_logf(const char *fmt, ...);

/**
 * Convert a pixel coordinate to a terminal cell coordinate.
 */
void nspire_px_to_cell(int px, int py, int *col, int *row);

#endif /* NSPIRE_DRAW_H */
