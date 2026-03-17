/**
 * \file nspire-draw.c
 * \brief Framebuffer drawing routines for the TI-Nspire CX II (Ndless)
 *
 * Maintains a 320×240 RGB-565 software framebuffer in RAM.
 * Characters are rendered from an embedded 5×8 bitmap font;
 * we only use the first 4 columns of each glyph so that the
 * terminal is exactly 80 columns wide at 320 px.
 *
 * Call nspire_video_flush() to blit the buffer to the physical LCD.
 */

#include "nspire-draw.h"

#include <libndls.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Software framebuffer
 * -------------------------------------------------------------------- */
/* One extra element absorbs the harmless out-of-bounds write that occurs
 * when drawing col 79 with the +1 screen-edge offset (x reaches 320). */
static nspire_pixel_t framebuf[NSPIRE_SCREEN_W * NSPIRE_SCREEN_H + 1];

/* -----------------------------------------------------------------------
 * Embedded 5×8 bitmap font (ASCII 0x00–0xFF)
 *
 * Each glyph is stored as FONT_H rows of FONT_SRC_W bytes (row-major,
 * left-to-right).  A byte value of 1 means "foreground pixel", 0 means
 * "background pixel".
 *
 * We draw only the first NSPIRE_FONT_W (=4) columns, making the cell
 * 4×8 pixels.  This exactly matches the NDS 5×8 font's visual content
 * because the 5th column is empty for virtually every ASCII glyph.
 * -------------------------------------------------------------------- */
#define FONT_SRC_W  5
#define FONT_SRC_H  8

#define EMPTY_ROW   0,0,0,0,0
#define EMPTY_GLYPH EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, \
                    EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW

static const uint8_t nspire_font_data[256 * FONT_SRC_W * FONT_SRC_H] = {
/* 0x00–0x1F  (control chars – all blank) */
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,

/* 0x20 (SPACE) */
EMPTY_GLYPH,

/* 0x21 (!) */
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x22 (") */
0,0,0,0,0,
0,1,0,1,0,
0,1,0,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x23 (#) */
0,0,0,0,0,
0,1,0,1,0,
0,1,0,1,0,
1,1,1,1,0,
0,1,0,1,0,
1,1,1,1,0,
0,1,0,1,0,
0,0,0,0,0,

/* 0x24 ($) */
0,0,1,0,0,
0,1,1,1,0,
1,0,1,0,0,
0,1,1,1,0,
0,0,1,1,0,
0,1,1,1,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x25 (%) */
0,0,0,0,0,
0,1,0,0,0,
0,1,0,1,0,
0,0,1,0,0,
0,1,0,0,0,
1,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x26 (&) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x27 (') */
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x28 (() */
0,0,0,0,0,
0,0,1,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x29 ()) */
0,0,0,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x2A (*) */
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
0,1,1,0,0,
1,1,1,1,0,
0,1,1,0,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x2B (+) */
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
1,1,1,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x2C (,) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,

/* 0x2D (-) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x2E (.) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x2F (/) */
0,0,0,0,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,0,0,0,

/* 0x30 (0) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,1,1,0,
1,1,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x31 (1) */
0,0,0,0,0,
0,0,1,0,0,
0,1,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x32 (2) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
0,0,1,0,0,
0,1,0,0,0,
1,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x33 (3) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
0,0,1,0,0,
0,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x34 (4) */
0,0,0,0,0,
1,0,1,0,0,
1,0,1,0,0,
1,0,1,0,0,
1,1,1,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x35 (5) */
0,0,0,0,0,
1,1,1,1,0,
1,0,0,0,0,
1,1,1,0,0,
0,0,0,1,0,
0,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x36 (6) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x37 (7) */
0,0,0,0,0,
1,1,1,1,0,
1,0,0,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,0,0,0,

/* 0x38 (8) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x39 (9) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x3A (:) */
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x3B (;) */
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,1,0,0,
0,1,0,0,0,
0,0,0,0,0,

/* 0x3C (<) */
0,0,0,0,0,
0,0,0,1,0,
0,0,1,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x3D (=) */
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
0,0,0,0,0,
0,1,1,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x3E (>) */
0,0,0,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,1,0,0,
0,1,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x3F (?) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
0,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,
0,1,0,0,0,
0,0,0,0,0,

/* 0x40 (@) */
0,1,1,0,0,
1,0,0,1,0,
1,0,1,1,0,
1,0,1,1,0,
1,0,0,0,0,
0,1,1,1,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x41 (A) */
0,0,0,0,0,
0,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x42 (B) */
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x43 (C) */
0,0,0,0,0,
0,1,1,1,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x44 (D) */
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x45 (E) */
0,0,0,0,0,
1,1,1,1,0,
1,0,0,0,0,
1,1,1,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x46 (F) */
0,0,0,0,0,
1,1,1,1,0,
1,0,0,0,0,
1,1,1,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
0,0,0,0,0,

/* 0x47 (G) */
0,0,0,0,0,
0,1,1,1,0,
1,0,0,0,0,
1,0,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x48 (H) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x49 (I) */
0,0,0,0,0,
0,1,1,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x4A (J) */
0,0,0,0,0,
0,0,1,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x4B (K) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x4C (L) */
0,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x4D (M) */
0,0,0,0,0,
1,0,0,1,0,
1,1,1,1,0,
1,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x4E (N) */
0,0,0,0,0,
1,0,0,1,0,
1,1,0,1,0,
1,0,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x4F (O) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x50 (P) */
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,0,0,
1,0,0,0,0,
0,0,0,0,0,

/* 0x51 (Q) */
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,0,1,0,
1,0,1,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x52 (R) */
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x53 (S) */
0,0,0,0,0,
0,1,1,1,0,
1,0,0,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x54 (T) */
0,0,0,0,0,
1,1,1,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x55 (U) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x56 (V) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,1,0,0,
1,1,0,0,0,
1,0,0,0,0,
0,0,0,0,0,

/* 0x57 (W) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,1,0,
1,1,1,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x58 (X) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x59 (Y) */
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x5A (Z) */
0,0,0,0,0,
1,1,1,1,0,
0,0,0,1,0,
0,0,1,0,0,
0,1,0,0,0,
1,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x5B ([) */
0,0,0,0,0,
0,1,1,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x5C (\) */
0,0,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,0,0,

/* 0x5D (]) */
0,0,0,0,0,
0,0,1,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,0,1,0,
0,0,1,1,0,
0,0,0,0,0,

/* 0x5E (^) */
0,0,0,0,0,
0,0,1,0,0,
0,1,0,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x5F (_) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x60 (`) */
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x61 (a) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x62 (b) */
0,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x63 (c) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
1,0,0,0,0,
1,0,0,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x64 (d) */
0,0,0,0,0,
0,0,0,1,0,
0,0,0,1,0,
0,1,1,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x65 (e) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,0,0,
1,0,1,1,0,
1,1,0,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x66 (f) */
0,0,0,0,0,
0,0,1,1,0,
0,1,0,0,0,
1,1,1,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,0,0,0,

/* 0x67 (g) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,1,0,
0,1,1,0,0,

/* 0x68 (h) */
0,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x69 (i) */
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,1,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x6A (j) */
0,0,0,0,0,
0,0,1,0,0,
0,0,0,0,0,
0,1,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,

/* 0x6B (k) */
0,0,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x6C (l) */
0,0,0,0,0,
0,1,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x6D (m) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,1,0,
1,0,1,1,0,
1,0,1,1,0,
1,0,1,1,0,
0,0,0,0,0,

/* 0x6E (n) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x6F (o) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x70 (p) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,0,0,
1,0,0,1,0,
1,1,1,0,0,
1,0,0,0,0,
1,0,0,0,0,

/* 0x71 (q) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,1,0,
0,0,0,1,0,

/* 0x72 (r) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,1,1,0,
1,1,0,0,0,
1,0,0,0,0,
1,0,0,0,0,
0,0,0,0,0,

/* 0x73 (s) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,1,1,1,0,
1,1,0,0,0,
0,0,1,1,0,
1,1,1,0,0,
0,0,0,0,0,

/* 0x74 (t) */
0,0,0,0,0,
0,0,0,0,0,
0,1,0,0,0,
0,1,1,1,0,
0,1,0,0,0,
0,1,0,0,0,
0,0,1,1,0,
0,0,0,0,0,

/* 0x75 (u) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,0,0,

/* 0x76 (v) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,0,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x77 (w) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
1,0,1,1,0,
0,1,1,0,0,
0,0,0,0,0,

/* 0x78 (x) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
0,1,1,0,0,
0,1,1,0,0,
1,0,0,1,0,
0,0,0,0,0,

/* 0x79 (y) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,0,0,1,0,
1,0,0,1,0,
0,1,1,1,0,
0,0,0,1,0,
0,1,1,0,0,

/* 0x7A (z) */
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
1,1,1,1,0,
0,0,1,0,0,
0,1,0,0,0,
1,1,1,1,0,
0,0,0,0,0,

/* 0x7B ({) */
0,0,0,0,0,
0,0,0,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,1,0,

/* 0x7C (|) */
0,0,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,0,0,

/* 0x7D (}) */
0,0,0,0,0,
0,1,0,0,0,
0,0,1,0,0,
0,0,1,0,0,
0,0,0,1,0,
0,0,1,0,0,
0,0,1,0,0,
0,1,0,0,0,

/* 0x7E (~) */
0,0,0,0,0,
0,1,0,1,0,
1,0,1,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,

/* 0x7F–0xFF: blank */
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH, EMPTY_GLYPH,
};

/* -----------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------- */

/** Write one pixel into the framebuffer (bounds-checked in debug). */
static inline void fb_put(int x, int y, nspire_pixel_t colour)
{
    if ((unsigned)x < NSPIRE_SCREEN_W && (unsigned)y < NSPIRE_SCREEN_H)
        framebuf[y * NSPIRE_SCREEN_W + x] = colour;
}

/* -----------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------- */

void nspire_video_init(void)
{
    lcd_init(SCR_320x240_565);
    memset(framebuf, 0, sizeof(framebuf));
}

void nspire_video_flush(void)
{
    lcd_blit(framebuf, SCR_320x240_565);
}

void nspire_clear(nspire_pixel_t colour)
{
    /* Fill as 32-bit pairs — twice the throughput of a 16-bit loop on
     * the ARM926EJ-S.  The framebuffer size is always a multiple of 4
     * bytes so this is safe. */
    uint32_t fill = (uint32_t)colour | ((uint32_t)colour << 16);
    uint32_t *p   = (uint32_t *)framebuf;
    uint32_t *end = p + (NSPIRE_SCREEN_W * NSPIRE_SCREEN_H / 2);
    while (p < end)
        *p++ = fill;
}

void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg)
{
    if ((unsigned)c > 0xFF)
        c = 0x20;

    const uint8_t *glyph = nspire_font_data
                           + (unsigned)c * (FONT_SRC_W * FONT_SRC_H);

    /* Compute the framebuffer destination pointer once for the whole cell.
     * Stepping by NSPIRE_SCREEN_W each row avoids a multiply-per-pixel.
     * The +1 matches the original pel offset that keeps column 0 clear of
     * the left bezel.  col 79 therefore reaches x=320; the framebuf has one
     * spare element allocated to absorb that write safely. */
    nspire_pixel_t *dst = framebuf
                          + row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                          + col * NSPIRE_FONT_W + 1;

    for (int gy = 0; gy < FONT_SRC_H; gy++, dst += NSPIRE_SCREEN_W) {
        /* Walk the 5-wide glyph row; only render the first 4 columns. */
        const uint8_t *src = glyph + gy * FONT_SRC_W;
        dst[0] = src[0] ? fg : bg;
        dst[1] = src[1] ? fg : bg;
        dst[2] = src[2] ? fg : bg;
        dst[3] = src[3] ? fg : bg;
    }
}

void nspire_draw_cursor(int col, int row)
{
    int px0 = col  * NSPIRE_FONT_W + 1;   /* +1: left bezel offset */
    int py0 = row  * NSPIRE_FONT_H;
    int px1 = px0  + NSPIRE_FONT_W - 1;
    int py1 = py0  + NSPIRE_FONT_H - 1;

    /* Top row */
    nspire_pixel_t *top = framebuf + py0 * NSPIRE_SCREEN_W + px0;
    top[0] = top[1] = top[2] = top[3] = NSPIRE_CURSOR;

    /* Bottom row */
    nspire_pixel_t *bot = framebuf + py1 * NSPIRE_SCREEN_W + px0;
    bot[0] = bot[1] = bot[2] = bot[3] = NSPIRE_CURSOR;

    /* Left / right vertical bars (skip corners already set) */
    for (int y = py0 + 1; y < py1; y++) {
        nspire_pixel_t *r = framebuf + y * NSPIRE_SCREEN_W + px0;
        r[0]  = NSPIRE_CURSOR;   /* left edge  */
        r[px1 - px0] = NSPIRE_CURSOR;   /* right edge */
    }
}

void nspire_log(const char *msg)
{
    static int log_col = 0, log_row = 0;

    for (; *msg; msg++) {
        if (*msg == '\n' || log_col >= NSPIRE_TERM_COLS) {
            log_col = 0;
            log_row++;
            if (*msg == '\n') continue;
        }
        if (log_row >= NSPIRE_TERM_ROWS)
            log_row = 0;
        nspire_draw_char(log_col++, log_row, *msg, NSPIRE_WHITE, NSPIRE_BLACK);
    }
    nspire_video_flush();
}

void nspire_logf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    char buf[len + 1];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    nspire_log(buf);
}

void nspire_px_to_cell(int px, int py, int *col, int *row)
{
    *col = px / NSPIRE_FONT_W;
    *row = py / NSPIRE_FONT_H;
}
