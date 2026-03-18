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
static nspire_pixel_t framebuf[NSPIRE_SCREEN_W * NSPIRE_SCREEN_H];

/* Dirty flag: set whenever the framebuffer is modified, cleared when
 * flushed to the LCD.  Avoids the costly lcd_blit() when nothing changed. */
static bool s_framebuf_dirty = false;

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

/* -----------------------------------------------------------------------
 * Compact glyph mask table (built at init from nspire_font_data)
 *
 * For each of the 256 glyphs, one byte per row encodes which of the
 * NSPIRE_FONT_W (=4) drawn columns is a foreground pixel:
 *   bit 3 = column 0  (leftmost)
 *   bit 2 = column 1
 *   bit 1 = column 2
 *   bit 0 = column 3  (rightmost)
 *
 * 256×8 = 2048 bytes vs 256×5×8 = 10240 bytes — 5× smaller working set
 * → better D-cache hit rate during heavily-tiled redraws.
 * -------------------------------------------------------------------- */
static uint8_t s_glyph_mask[256][FONT_SRC_H];

/* -----------------------------------------------------------------------
 * Horizontal drawing offset (pixels)
 *
 * The Nspire LCD bezel clips the leftmost screen column, causing
 * characters whose glyph starts in column 0 (e.g. 'L', 'P') to have
 * their left edge cut off.  Shifting every draw operation one pixel to
 * the right keeps all glyphs fully visible.
 *
 * NOTE: this offset makes positions no longer 4-byte aligned, so all
 * drawing loops below use individual 16-bit (nspire_pixel_t) stores
 * rather than packed 32-bit stores.
 * -------------------------------------------------------------------- */
#define NSPIRE_X_OFFSET 1

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

/** Write one pixel into the framebuffer (bounds-checked). */
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

    /* Build compact glyph mask table from the byte-per-pixel font data.
     * Row byte format: bit 3 = col 0, bit 2 = col 1, bit 1 = col 2,
     *                  bit 0 = col 3.                                  */
    for (int i = 0; i < 256; i++) {
        const uint8_t *g = nspire_font_data + i * (FONT_SRC_W * FONT_SRC_H);
        for (int gy = 0; gy < FONT_SRC_H; gy++) {
            const uint8_t *row = g + gy * FONT_SRC_W;
            s_glyph_mask[i][gy] = (uint8_t)(
                (row[0] ? 0x08u : 0u) |
                (row[1] ? 0x04u : 0u) |
                (row[2] ? 0x02u : 0u) |
                (row[3] ? 0x01u : 0u));
        }
    }
}

void nspire_video_flush(void)
{
    if (!s_framebuf_dirty)
        return;
    lcd_blit(framebuf, SCR_320x240_565);
    s_framebuf_dirty = false;
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
    s_framebuf_dirty = true;
}

void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg)
{
    if ((unsigned)c > 0xFF)
        c = 0x20;

    /* Use the compact glyph mask (1 byte/row) rather than the 5-byte-per-row
     * source table.  This is 5× smaller, fitting more glyphs in D-cache.
     *
     * NSPIRE_X_OFFSET shifts the glyph one pixel right so that glyphs
     * starting in column 0 (e.g. 'L', 'P') are not clipped by the bezel.
     * The offset makes dst no longer 4-byte aligned, so we use individual
     * 16-bit stores instead of packed 32-bit stores. */
    const uint8_t *mask = s_glyph_mask[(unsigned char)c];
    nspire_pixel_t *dst = framebuf
                          + row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                          + col * NSPIRE_FONT_W
                          + NSPIRE_X_OFFSET; /* 1-pixel left-bezel compensation */

    for (int gy = 0; gy < FONT_SRC_H; gy++, dst += NSPIRE_SCREEN_W) {
        /* Derive 4 pixel values from bits 3..0 and write individually.
         * Clip the last pixel when it would fall outside the framebuffer
         * (can happen for the rightmost column due to the X offset). */
        uint8_t m = mask[gy];
        nspire_pixel_t px[4] = {
            (m & 0x08) ? fg : bg,
            (m & 0x04) ? fg : bg,
            (m & 0x02) ? fg : bg,
            (m & 0x01) ? fg : bg,
        };
        int end_px = col * NSPIRE_FONT_W + NSPIRE_X_OFFSET + NSPIRE_FONT_W;
        int draw   = (end_px <= NSPIRE_SCREEN_W) ? NSPIRE_FONT_W
                                                 : NSPIRE_FONT_W - (end_px - NSPIRE_SCREEN_W);
        for (int i = 0; i < draw; i++)
            dst[i] = px[i];
    }
    s_framebuf_dirty = true;
}

void nspire_fill_cells(int col, int row, int ncols, nspire_pixel_t colour)
{
    /* Bulk-fill ncols terminal cells on the given row with a solid colour.
     * Used by the Term_wipe hook to avoid the per-character overhead of
     * nspire_draw_char when erasing spans of cells.
     *
     * Applies NSPIRE_X_OFFSET so that the erased region matches where
     * nspire_draw_char places glyphs.  Individual 16-bit stores are used
     * because the offset makes the address no longer 4-byte aligned. */
    int width_px = ncols * NSPIRE_FONT_W;
    int base_idx = row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                   + col * NSPIRE_FONT_W
                   + NSPIRE_X_OFFSET; /* 1-pixel left-bezel compensation */
    /* Clip width to stay within the framebuffer */
    int avail = NSPIRE_SCREEN_W - (col * NSPIRE_FONT_W + NSPIRE_X_OFFSET);
    if (width_px > avail)
        width_px = avail;

    for (int gy = 0; gy < NSPIRE_FONT_H; gy++) {
        nspire_pixel_t *r = framebuf + base_idx + gy * NSPIRE_SCREEN_W;
        for (int i = 0; i < width_px; i++)
            r[i] = colour;
    }
    s_framebuf_dirty = true;
}


void nspire_draw_cursor(int col, int row)
{
    /* Apply NSPIRE_X_OFFSET so the cursor box aligns with the drawn glyphs. */
    int px0 = col  * NSPIRE_FONT_W + NSPIRE_X_OFFSET; /* 1-pixel left-bezel compensation */
    int py0 = row  * NSPIRE_FONT_H;
    int px1 = px0  + NSPIRE_FONT_W - 1;
    int py1 = py0  + NSPIRE_FONT_H - 1;
    /* Clip px1 to the last valid screen column */
    if (px1 >= NSPIRE_SCREEN_W)
        px1 = NSPIRE_SCREEN_W - 1;

    /* Top row */
    nspire_pixel_t *top = framebuf + py0 * NSPIRE_SCREEN_W + px0;
    for (int i = 0; i <= px1 - px0; i++) top[i] = NSPIRE_CURSOR;

    /* Bottom row */
    nspire_pixel_t *bot = framebuf + py1 * NSPIRE_SCREEN_W + px0;
    for (int i = 0; i <= px1 - px0; i++) bot[i] = NSPIRE_CURSOR;

    /* Left / right vertical bars (skip corners already set) */
    for (int y = py0 + 1; y < py1; y++) {
        nspire_pixel_t *r = framebuf + y * NSPIRE_SCREEN_W + px0;
        r[0]           = NSPIRE_CURSOR;   /* left edge  */
        r[px1 - px0]   = NSPIRE_CURSOR;   /* right edge */
    }
    s_framebuf_dirty = true;
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
