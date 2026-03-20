/**
 * \file nspire/nspire-draw.c
 * \brief Framebuffer drawing routines for the TI-Nspire CX II — BrogueCE port
 *
 * Maintains a 320×240 RGB-565 software framebuffer in RAM.
 * Characters are rendered from an embedded 5×8 NDS-style bitmap font;
 * we use columns 0-3 and rows 0-7 of each glyph to produce 4×8 cells,
 * fitting BrogueCE's 80×30 terminal exactly in the 320×240 screen.
 *
 * Call nspire_video_flush() to blit the buffer to the LCD.
 */

#include "nspire-draw.h"

#include <libndls.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Aggressive scalar/store optimisations for the 150 MHz ARM. */
#pragma GCC optimize("O3,unroll-loops")

/* -----------------------------------------------------------------------
 * Software framebuffer
 * -------------------------------------------------------------------- */
static nspire_pixel_t framebuf[NSPIRE_SCREEN_W * NSPIRE_SCREEN_H];

/* 64-bit dirty-row bitmask: bit y set → terminal row y needs flushing.
 * 34 rows fit comfortably within a uint64_t.                           */
static uint64_t s_dirty_rows = 0u;
static bool     s_framebuf_dirty = false;

/* -----------------------------------------------------------------------
 * Embedded 5×8 bitmap font (NDS-style, ASCII 0x00–0xFF)
 *
 * Each glyph is 5 columns × 8 rows (row-major, left-to-right).
 * A byte value of 1 = foreground pixel, 0 = background pixel.
 *
 * We render only columns 1-3 of rows 1-7, producing a 3×7 cell that
 * fits BrogueCE's 100×34 terminal in the 320×240 Nspire screen.
 * -------------------------------------------------------------------- */
#define FONT_SRC_W  5
#define FONT_SRC_H  8

#define EMPTY_ROW   0,0,0,0,0
#define EMPTY_GLYPH EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, \
                    EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW

/* Compact glyph mask table: for each of 256 glyphs, one byte per
 * rendered row (8 rows) encodes which of the NSPIRE_FONT_W (=4)
 * drawn columns is a foreground pixel:
 *   bit 3 = column 0  (leftmost drawn)
 *   bit 2 = column 1
 *   bit 1 = column 2
 *   bit 0 = column 3  (rightmost drawn)
 * Built at init from nspire_font_data by taking src cols 0-3, rows 0-7. */
static uint8_t s_glyph_mask[256][NSPIRE_FONT_H];

static const uint8_t nspire_font_data[256 * FONT_SRC_W * FONT_SRC_H] = {
#include "../nspire-font-5x8.inc"
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

    /* Build compact glyph mask: src cols 0-3, src rows 0-7.
     * Row byte format: bit 3 = src col 0, bit 2 = src col 1,
     *                  bit 1 = src col 2, bit 0 = src col 3.    */
    for (int i = 0; i < 256; i++) {
        /* Pointer to this glyph's first row in the source data. */
        const uint8_t *g = nspire_font_data + i * (FONT_SRC_W * FONT_SRC_H);
        for (int gy = 0; gy < NSPIRE_FONT_H; gy++) {
            const uint8_t *row = g + gy * FONT_SRC_W;
            s_glyph_mask[i][gy] = (uint8_t)(
                (row[0] ? 0x08u : 0u) |   /* src col 0 → dst col 0 */
                (row[1] ? 0x04u : 0u) |   /* src col 1 → dst col 1 */
                (row[2] ? 0x02u : 0u) |   /* src col 2 → dst col 2 */
                (row[3] ? 0x01u : 0u));   /* src col 3 → dst col 3 */
        }
    }
}

void nspire_video_flush(void)
{
    if (!s_framebuf_dirty)
        return;
    s_framebuf_dirty = false;
    s_dirty_rows = 0u;

    /* Copy the entire software framebuffer to the physical LCD.
     * Reading REAL_SCREEN_BASE_ADDRESS once avoids any tearing from a
     * mid-copy OS buffer flip; the single bulk memcpy is faster than
     * per-row copies on the ARM926EJ-S write buffer.              */
    void *hw = REAL_SCREEN_BASE_ADDRESS;
    memcpy(hw, framebuf, sizeof(framebuf));
}

void nspire_clear(nspire_pixel_t colour)
{
    uint32_t fill = (uint32_t)colour | ((uint32_t)colour << 16);
    uint32_t *p   = (uint32_t *)framebuf;
    uint32_t *end = p + (NSPIRE_SCREEN_W * NSPIRE_SCREEN_H / 2);
    while (p < end)
        *p++ = fill;
    /* All 34 terminal rows are dirty. */
    s_dirty_rows = (1ull << NSPIRE_TERM_ROWS) - 1ull;
    s_framebuf_dirty = true;
}

void nspire_draw_char(int col, int row, int c,
                      nspire_pixel_t fg, nspire_pixel_t bg)
{
    if ((unsigned)c > 0xFF || c < 0x20)
        c = 0x20;  /* Render unknown/control chars as space */

    nspire_pixel_t *dst = framebuf
                          + row * NSPIRE_FONT_H * NSPIRE_SCREEN_W
                          + col * NSPIRE_FONT_W
                          + NSPIRE_X_OFFSET;

    /* Clip check: only the rightmost terminal column can overflow. */
    const int end_px = col * NSPIRE_FONT_W + NSPIRE_X_OFFSET + NSPIRE_FONT_W;
    const int clipped = (end_px > NSPIRE_SCREEN_W);
    const int draw    = clipped
                        ? NSPIRE_FONT_W - (end_px - NSPIRE_SCREEN_W)
                        : NSPIRE_FONT_W;

    /* Fast path: invisible glyph (fg == bg) — solid fill the cell. */
    if (fg == bg) {
        for (int gy = 0; gy < NSPIRE_FONT_H; gy++) {
            nspire_pixel_t *r = dst + gy * NSPIRE_SCREEN_W;
            for (int i = 0; i < draw; i++) r[i] = fg;
        }
        s_dirty_rows |= (1ull << row);
        s_framebuf_dirty = true;
        return;
    }

    /* Normal path: look up compact glyph mask (256 × 7 = 1792 bytes,
     * fits comfortably in D-cache).                                   */
    const uint8_t *mask = s_glyph_mask[(unsigned char)c];

    if (!clipped) {
        /* Common case: all 4 pixels fit.
         * One 16-bit store per pixel (4 per row, 8 rows = 32 stores).  */
#define NSPIRE_DRAW_ROW_4(gy) do { \
    uint8_t _m = mask[gy]; \
    dst[(gy)*NSPIRE_SCREEN_W + 0] = (_m & 0x08) ? fg : bg; \
    dst[(gy)*NSPIRE_SCREEN_W + 1] = (_m & 0x04) ? fg : bg; \
    dst[(gy)*NSPIRE_SCREEN_W + 2] = (_m & 0x02) ? fg : bg; \
    dst[(gy)*NSPIRE_SCREEN_W + 3] = (_m & 0x01) ? fg : bg; \
} while (0)
        NSPIRE_DRAW_ROW_4(0); NSPIRE_DRAW_ROW_4(1); NSPIRE_DRAW_ROW_4(2);
        NSPIRE_DRAW_ROW_4(3); NSPIRE_DRAW_ROW_4(4); NSPIRE_DRAW_ROW_4(5);
        NSPIRE_DRAW_ROW_4(6); NSPIRE_DRAW_ROW_4(7);
#undef NSPIRE_DRAW_ROW_4
    } else {
        /* Clipped: write only as many pixels as fit. */
        for (int gy = 0; gy < NSPIRE_FONT_H; gy++) {
            uint8_t m = mask[gy];
            nspire_pixel_t *r = dst + gy * NSPIRE_SCREEN_W;
            if (draw > 0) r[0] = (m & 0x08) ? fg : bg;
            if (draw > 1) r[1] = (m & 0x04) ? fg : bg;
            if (draw > 2) r[2] = (m & 0x02) ? fg : bg;
            if (draw > 3) r[3] = (m & 0x01) ? fg : bg;
        }
    }
    s_dirty_rows |= (1ull << row);
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
