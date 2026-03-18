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

/* Ask GCC to apply the most aggressive scalar/store optimisations to every
 * function in this translation unit.  This is critical on the 150 MHz ARM
 * of the Nspire CX II where the framebuffer bandwidth is the bottleneck. */
#pragma GCC optimize("O3,unroll-loops")

/* -----------------------------------------------------------------------
 * Software framebuffer
 * -------------------------------------------------------------------- */
static nspire_pixel_t framebuf[NSPIRE_SCREEN_W * NSPIRE_SCREEN_H];

/* Dirty flag: set whenever the framebuffer is modified, cleared when
 * flushed to the LCD.  Avoids the costly lcd_blit() when nothing changed. */
static bool     s_framebuf_dirty = false;
/* Bitmask of dirty terminal rows (bit y set = row y needs flushing).
 * 30 rows fit in a single uint32_t.  Set by every draw function, cleared
 * by nspire_video_flush() after the LCD copy. */
static uint32_t s_dirty_rows = 0u;

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
    s_framebuf_dirty = false;

    uint8_t *hw = (uint8_t *)REAL_SCREEN_BASE_ADDRESS;
    const uint8_t *sw = (const uint8_t *)framebuf;
    /* One terminal row spans NSPIRE_FONT_H pixel rows, each NSPIRE_SCREEN_W
     * pixels wide (2 bytes each). */
    const int row_stride = NSPIRE_FONT_H * NSPIRE_SCREEN_W
                           * (int)sizeof(nspire_pixel_t);
    uint32_t dirty = s_dirty_rows;
    s_dirty_rows = 0u;

    /* Merge contiguous dirty terminal rows into a single memcpy per run.
     * This maximises AHB burst-write efficiency and reduces call overhead
     * compared to 30 individual row copies. */
    while (dirty) {
        int start = __builtin_ctz(dirty);
        int len   = __builtin_ctz(~(dirty >> start));
        dirty &= ~(((1u << len) - 1u) << start);
        memcpy(hw + start * row_stride,
               sw + start * row_stride,
               (size_t)len * row_stride);
    }
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
    /* Mark every terminal row dirty so nspire_video_flush copies the whole screen. */
    s_dirty_rows = (1u << NSPIRE_TERM_ROWS) - 1u;
    s_framebuf_dirty = true;
}

void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg)
{
    if ((unsigned)c > 0xFF)
        c = 0x20;

    nspire_pixel_t *dst = framebuf
                          + row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                          + col * NSPIRE_FONT_W
                          + NSPIRE_X_OFFSET; /* 1-pixel left-bezel compensation */

    /* Hoist clipping check: only the very last terminal column can overflow.
     * Computing it once here instead of inside the 8-row loop saves ~7
     * redundant multiply-add sequences per character draw. */
    const int end_px = col * NSPIRE_FONT_W + NSPIRE_X_OFFSET + NSPIRE_FONT_W;
    const int clipped = (end_px > NSPIRE_SCREEN_W);
    const int draw    = clipped ? NSPIRE_FONT_W - (end_px - NSPIRE_SCREEN_W)
                                : NSPIRE_FONT_W;

    /* Fast path: fg == bg means the glyph is invisible – just solid-fill
     * the cell.  This covers BG_SAME tiles and avoids a mask table lookup. */
    if (fg == bg) {
        const uint32_t fill = ((uint32_t)fg << 16) | fg;
        if (!clipped) {
            /* 2× 32-bit stores per row, 8 rows fully unrolled */
#define NSPIRE_FILL_ROW(gy) do { \
    __builtin_memcpy(dst + (gy)*NSPIRE_SCREEN_W,     &fill, 4); \
    __builtin_memcpy(dst + (gy)*NSPIRE_SCREEN_W + 2, &fill, 4); \
} while (0)
            NSPIRE_FILL_ROW(0); NSPIRE_FILL_ROW(1);
            NSPIRE_FILL_ROW(2); NSPIRE_FILL_ROW(3);
            NSPIRE_FILL_ROW(4); NSPIRE_FILL_ROW(5);
            NSPIRE_FILL_ROW(6); NSPIRE_FILL_ROW(7);
#undef NSPIRE_FILL_ROW
        } else {
            for (int gy = 0; gy < FONT_SRC_H; gy++, dst += NSPIRE_SCREEN_W)
                for (int i = 0; i < draw; i++) dst[i] = fg;
        }
        s_dirty_rows |= 1u << row;
        s_framebuf_dirty = true;
        return;
    }

    /* Normal path: look up the compact 1-byte-per-row glyph mask.
     * 256×8 = 2 KB working set fits comfortably in the D-cache. */
    const uint8_t *mask = s_glyph_mask[(unsigned char)c];

    /* Precompute all four possible 2-pixel packed words (one per combination
     * of two adjacent pixels being fg or bg).
     * On little-endian ARM a 32-bit store at p writes:
     *   p[0] = low  16 bits   (first  pixel)
     *   p[1] = high 16 bits   (second pixel)
     * so:  pair[0b_col1_col0] = ((uint32_t)col1_px << 16) | col0_px
     *
     * Index bit layout:  bit1 = first pixel (1→fg, 0→bg)
     *                    bit0 = second pixel (1→fg, 0→bg)           */
    const uint32_t pair[4] = {
        ((uint32_t)bg << 16) | bg,   /* 00 → p[0]=bg, p[1]=bg */
        ((uint32_t)fg << 16) | bg,   /* 01 → p[0]=bg, p[1]=fg */
        ((uint32_t)bg << 16) | fg,   /* 10 → p[0]=fg, p[1]=bg */
        ((uint32_t)fg << 16) | fg,   /* 11 → p[0]=fg, p[1]=fg */
    };

    if (!clipped) {
        /* Common case: all 4 pixels fit in the framebuffer.
         * Two 32-bit packed stores per row replace four 16-bit stores.
         * The entire 8-row loop is unrolled so the compiler can schedule
         * the independent loads/stores and keep pair[] in registers.    */
#define NSPIRE_DRAW_ROW(gy) do { \
    uint8_t _m = mask[gy]; \
    uint32_t _w0 = pair[(_m >> 2) & 3];  /* cols 0,1 */ \
    uint32_t _w1 = pair[ _m       & 3];  /* cols 2,3 */ \
    __builtin_memcpy(dst + (gy)*NSPIRE_SCREEN_W,     &_w0, 4); \
    __builtin_memcpy(dst + (gy)*NSPIRE_SCREEN_W + 2, &_w1, 4); \
} while (0)
        NSPIRE_DRAW_ROW(0); NSPIRE_DRAW_ROW(1);
        NSPIRE_DRAW_ROW(2); NSPIRE_DRAW_ROW(3);
        NSPIRE_DRAW_ROW(4); NSPIRE_DRAW_ROW(5);
        NSPIRE_DRAW_ROW(6); NSPIRE_DRAW_ROW(7);
#undef NSPIRE_DRAW_ROW
    } else {
        /* Clipped case: only the rightmost terminal column ever reaches here;
         * write 1–3 pixels with individual 16-bit stores. */
        for (int gy = 0; gy < FONT_SRC_H; gy++, dst += NSPIRE_SCREEN_W) {
            uint8_t m = mask[gy];
            dst[0] = (m & 0x08) ? fg : bg;
            if (draw > 1) dst[1] = (m & 0x04) ? fg : bg;
            if (draw > 2) dst[2] = (m & 0x02) ? fg : bg;
        }
    }
    s_dirty_rows |= 1u << row;
    s_framebuf_dirty = true;
}

void nspire_fill_cells(int col, int row, int ncols, nspire_pixel_t colour)
{
    /* Bulk-fill ncols terminal cells on the given row with a solid colour.
     * Used by the Term_wipe hook to avoid the per-character overhead of
     * nspire_draw_char when erasing spans of cells.
     *
     * With NSPIRE_X_OFFSET=1 the base pixel address is always at a 2-byte
     * boundary that is NOT 4-byte aligned (byte offset = 8*col+2, ≡2 mod 4).
     * We handle this with one leading 16-bit store to reach 4-byte alignment,
     * then 32-bit packed stores for the bulk of the row, then an optional
     * trailing 16-bit store.  This halves the number of memory transactions
     * vs the previous per-pixel 16-bit loop. */
    const int base_px  = col * NSPIRE_FONT_W + NSPIRE_X_OFFSET;
    const int avail    = NSPIRE_SCREEN_W - base_px;
    int       width_px = ncols * NSPIRE_FONT_W;
    if (width_px > avail) width_px = avail;
    if (width_px <= 0) return;

    const uint32_t fill32 = ((uint32_t)colour << 16) | colour;

    for (int gy = 0; gy < NSPIRE_FONT_H; gy++) {
        nspire_pixel_t *p = framebuf
                            + (row * NSPIRE_FONT_H + gy) * NSPIRE_SCREEN_W
                            + base_px;
        int n = width_px;

        /* One leading 16-bit store aligns the pointer to a 4-byte boundary
         * (base_px is always ≡2 mod 4, so this always fires). */
        if ((uintptr_t)p & 2) {
            *p++ = colour;
            n--;
        }

        /* 32-bit bulk fill: two pixels per store */
        uint32_t *w = (uint32_t *)p;
        int pairs = n >> 1;
        for (int i = 0; i < pairs; i++)
            w[i] = fill32;

        /* One optional trailing 16-bit store for an odd pixel count */
        if (n & 1)
            p[n - 1] = colour;
    }
    s_dirty_rows |= 1u << row;
    s_framebuf_dirty = true;
}

/**
 * Draw n characters from wide-char string s at terminal cell (col, row),
 * all sharing the same fg/bg colours (as batched by Term_fresh_row_both).
 *
 * Saves vs n × nspire_draw_char():
 *   • row_base pointer computed once (avoids n-1 multiply-add sequences)
 *   • pair[4] table computed once per span instead of per glyph
 *   • clipping only checked for the final glyph
 */
void nspire_draw_chars(int col, int row, int n,
                       nspire_pixel_t fg, nspire_pixel_t bg,
                       const wchar_t *s)
{
    /* Base address: first pixel of glyph 0.  Each subsequent glyph is
     * +i*NSPIRE_FONT_W from here — one add instead of a full MUL+ADD. */
    nspire_pixel_t *row_base = framebuf
                               + row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                               + col * NSPIRE_FONT_W
                               + NSPIRE_X_OFFSET;

    /* Only the very last glyph can have its right edge clipped. */
    const int last_end_px = (col + n - 1) * NSPIRE_FONT_W
                            + NSPIRE_X_OFFSET + NSPIRE_FONT_W;
    const int last_draw   = (last_end_px > NSPIRE_SCREEN_W)
                            ? NSPIRE_FONT_W - (last_end_px - NSPIRE_SCREEN_W)
                            : NSPIRE_FONT_W;
    const int last_clipped = (last_draw < NSPIRE_FONT_W);

    if (fg == bg) {
        /* Solid-colour span — no mask lookup needed at all. */
        const uint32_t fill = ((uint32_t)fg << 16) | fg;
        int i, gy;
        for (i = 0; i < n; i++) {
            nspire_pixel_t *d = row_base + i * NSPIRE_FONT_W;
            if (!last_clipped || i < n - 1) {
#define NSPIRE_FILL_ROWB(gy) do { \
    __builtin_memcpy(d + (gy)*NSPIRE_SCREEN_W,     &fill, 4); \
    __builtin_memcpy(d + (gy)*NSPIRE_SCREEN_W + 2, &fill, 4); \
} while (0)
                NSPIRE_FILL_ROWB(0); NSPIRE_FILL_ROWB(1);
                NSPIRE_FILL_ROWB(2); NSPIRE_FILL_ROWB(3);
                NSPIRE_FILL_ROWB(4); NSPIRE_FILL_ROWB(5);
                NSPIRE_FILL_ROWB(6); NSPIRE_FILL_ROWB(7);
#undef NSPIRE_FILL_ROWB
            } else {
                for (gy = 0; gy < FONT_SRC_H; gy++)
                    for (int k = 0; k < last_draw; k++)
                        d[gy * NSPIRE_SCREEN_W + k] = fg;
            }
        }
        s_dirty_rows |= 1u << row;
        s_framebuf_dirty = true;
        return;
    }

    /* Precompute the 4-entry pixel-pair table once for the entire span.
     * Bit layout identical to nspire_draw_char (see that function for
     * full commentary on the little-endian memcpy trick). */
    const uint32_t pair[4] = {
        ((uint32_t)bg << 16) | bg,
        ((uint32_t)fg << 16) | bg,
        ((uint32_t)bg << 16) | fg,
        ((uint32_t)fg << 16) | fg,
    };

    for (int i = 0; i < n; i++) {
        int c = (int)(s[i]);
        if ((unsigned)c > 0xFF) c = 0x20;
        const uint8_t *mask = s_glyph_mask[(unsigned char)c];
        nspire_pixel_t *d   = row_base + i * NSPIRE_FONT_W;

        if (!last_clipped || i < n - 1) {
#define NSPIRE_DRAW_ROWB(gy) do { \
    uint8_t _m = mask[gy]; \
    uint32_t _w0 = pair[(_m >> 2) & 3]; \
    uint32_t _w1 = pair[ _m       & 3]; \
    __builtin_memcpy(d + (gy)*NSPIRE_SCREEN_W,     &_w0, 4); \
    __builtin_memcpy(d + (gy)*NSPIRE_SCREEN_W + 2, &_w1, 4); \
} while (0)
            NSPIRE_DRAW_ROWB(0); NSPIRE_DRAW_ROWB(1);
            NSPIRE_DRAW_ROWB(2); NSPIRE_DRAW_ROWB(3);
            NSPIRE_DRAW_ROWB(4); NSPIRE_DRAW_ROWB(5);
            NSPIRE_DRAW_ROWB(6); NSPIRE_DRAW_ROWB(7);
#undef NSPIRE_DRAW_ROWB
        } else {
            /* Clipped final column: 1-3 pixels via individual stores. */
            for (int gy = 0; gy < FONT_SRC_H; gy++) {
                uint8_t m = mask[gy];
                d[gy * NSPIRE_SCREEN_W + 0] = (m & 0x08) ? fg : bg;
                if (last_draw > 1)
                    d[gy * NSPIRE_SCREEN_W + 1] = (m & 0x04) ? fg : bg;
                if (last_draw > 2)
                    d[gy * NSPIRE_SCREEN_W + 2] = (m & 0x02) ? fg : bg;
            }
        }
    }
    s_dirty_rows |= 1u << row;
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
    s_dirty_rows |= 1u << row;
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
