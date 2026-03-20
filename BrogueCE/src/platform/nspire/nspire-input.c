/**
 * \file nspire/nspire-input.c
 * \brief Keyboard polling and key mapping for the TI-Nspire CX II — BrogueCE port
 *
 * Maps the TI-Nspire CX II hardware keys to BrogueCE's expected key codes.
 * BrogueCE uses plain ASCII/Unicode integers for keystrokes, with separate
 * controlKey and shiftKey boolean flags.
 *
 * Movement mapping (vi/roguelike convention via numpad):
 *   Arrow keys     → UP_ARROW / DOWN_ARROW / LEFT_ARROW / RIGHT_ARROW
 *   Touchpad click → '5' (wait in place)  [run is shift+direction]
 *   Diagonal dirs  → '9','3','1','7' (numpad roguelike diagonals)
 *
 * Special keys:
 *   ESC   → ESCAPE_KEY  (0x1B)
 *   ENTER → '\r'
 *   DEL   → DELETE_KEY  (0x7F)
 *   TAB   → '\t'
 */

#include "nspire-input.h"
#include "nspire-event.h"

#include "Rogue.h"

#include <libndls.h>
#include <keys.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Hardware modifier state
 * -------------------------------------------------------------------- */
#define HW_MOD_SHIFT   0x01
#define HW_MOD_CTRL    0x02

/* -----------------------------------------------------------------------
 * Key repeat state
 * -------------------------------------------------------------------- */
typedef struct {
    const t_key  *key;          /**< currently held key (or NULL)       */
    signed long   code;         /**< key code that was emitted           */
    boolean       ctrl;         /**< control modifier at press time      */
    boolean       shift;        /**< shift modifier at press time        */
    uint32_t      press_tick;   /**< tick of first press                 */
    uint32_t      last_tick;    /**< tick of last repeat emission        */
    bool          repeating;    /**< has initial-delay elapsed?          */
} repeat_state_t;

static repeat_state_t s_repeat = {0};

/* Simple millisecond tick counter */
static uint32_t s_tick = 0;

/* -----------------------------------------------------------------------
 * Key table entry
 * -------------------------------------------------------------------- */
typedef struct {
    const t_key  *nspire_key;
    signed long   unshifted;   /**< code when SHIFT not held             */
    signed long   shifted;     /**< code when SHIFT held                 */
} key_entry_t;

/* -----------------------------------------------------------------------
 * Main key table — Nspire CX II layout
 * -------------------------------------------------------------------- */
static const key_entry_t s_key_table[] = {
    /* --- Letters --- */
    { &KEY_NSPIRE_A, 'a', 'A' },
    { &KEY_NSPIRE_B, 'b', 'B' },
    { &KEY_NSPIRE_C, 'c', 'C' },
    { &KEY_NSPIRE_D, 'd', 'D' },
    { &KEY_NSPIRE_E, 'e', 'E' },
    { &KEY_NSPIRE_F, 'f', 'F' },
    { &KEY_NSPIRE_G, 'g', 'G' },
    { &KEY_NSPIRE_H, 'h', 'H' },
    { &KEY_NSPIRE_I, 'i', 'I' },
    { &KEY_NSPIRE_J, 'j', 'J' },
    { &KEY_NSPIRE_K, 'k', 'K' },
    { &KEY_NSPIRE_L, 'l', 'L' },
    { &KEY_NSPIRE_M, 'm', 'M' },
    { &KEY_NSPIRE_N, 'n', 'N' },
    { &KEY_NSPIRE_O, 'o', 'O' },
    { &KEY_NSPIRE_P, 'p', 'P' },
    { &KEY_NSPIRE_Q, 'q', 'Q' },
    { &KEY_NSPIRE_R, 'r', 'R' },
    { &KEY_NSPIRE_S, 's', 'S' },
    { &KEY_NSPIRE_T, 't', 'T' },
    { &KEY_NSPIRE_U, 'u', 'U' },
    { &KEY_NSPIRE_V, 'v', 'V' },
    { &KEY_NSPIRE_W, 'w', 'W' },
    { &KEY_NSPIRE_X, 'x', 'X' },
    { &KEY_NSPIRE_Y, 'y', 'Y' },
    { &KEY_NSPIRE_Z, 'z', 'Z' },

    /* --- Digits (shifted → symbols) --- */
    { &KEY_NSPIRE_0, '0', ')' },
    { &KEY_NSPIRE_1, '1', '!' },
    { &KEY_NSPIRE_2, '2', '@' },
    { &KEY_NSPIRE_3, '3', '#' },
    { &KEY_NSPIRE_4, '4', '$' },
    { &KEY_NSPIRE_5, '5', '%' },
    { &KEY_NSPIRE_6, '6', '^' },
    { &KEY_NSPIRE_7, '7', '&' },
    { &KEY_NSPIRE_8, '8', '*' },
    { &KEY_NSPIRE_9, '9', '(' },

    /* --- Punctuation --- */
    { &KEY_NSPIRE_SPACE,      ' ',   ' '  },
    { &KEY_NSPIRE_PERIOD,     '.',   '>'  },
    { &KEY_NSPIRE_COMMA,      ',',   '<'  },
    { &KEY_NSPIRE_MINUS,      '-',   '_'  },
    { &KEY_NSPIRE_PLUS,       '+',   '='  },
    { &KEY_NSPIRE_LP,         '(',   '['  },
    { &KEY_NSPIRE_RP,         ')',   ']'  },
    { &KEY_NSPIRE_DIVIDE,     '/',   '/'  },
    { &KEY_NSPIRE_MULTIPLY,   '*',   '|'  },
    { &KEY_NSPIRE_EQU,        '=',   '+'  },
    { &KEY_NSPIRE_EXP,        '^',   '~'  },
    /* DOC key → '?' (CX II only; a dedicated question-mark key) */
    { &KEY_NSPIRE_DOC,        '?',   '?'  },
    /* QUESEXCL → colon / semicolon (CX II only) */
    { &KEY_NSPIRE_QUESEXCL,   ':',   ';'  },
    { &KEY_NSPIRE_APOSTROPHE, '\'',  '`'  },
    /* BAR key → | / backslash */
    { &KEY_NSPIRE_BAR,        '|',  '\\' },
    /* Negative sign key (same symbol as MINUS on most Nspire versions) */
    { &KEY_NSPIRE_NEGATIVE,   '-',   '_'  },

    /* --- Arrow keys --- */
    { &KEY_NSPIRE_UP,         UP_ARROW,    UP_ARROW    },
    { &KEY_NSPIRE_DOWN,       DOWN_ARROW,  DOWN_ARROW  },
    { &KEY_NSPIRE_LEFT,       LEFT_ARROW,  LEFT_ARROW  },
    { &KEY_NSPIRE_RIGHT,      RIGHT_ARROW, RIGHT_ARROW },

    /* --- Diagonal touchpad directions → numpad roguelike movement ---
     * NUMPAD_7/9/3/1 = '7'/'9'/'3'/'1' in BrogueCE (Rogue.h:1219-1228).
     * Shifted versions send uppercase vi-letters so BrogueCE detects
     * shiftKey=true and runs in that direction (4 steps).            */
    { &KEY_NSPIRE_UPRIGHT,    '9', 'U' },   /* NE: walk/run             */
    { &KEY_NSPIRE_RIGHTDOWN,  '3', 'N' },   /* SE: walk/run             */
    { &KEY_NSPIRE_DOWNLEFT,   '1', 'B' },   /* SW: walk/run             */
    { &KEY_NSPIRE_LEFTUP,     '7', 'Y' },   /* NW: walk/run             */

    /* --- Touchpad center click → wait in place --- */
    { &KEY_NSPIRE_CLICK,      '.',  '.' },

    /* --- Tab --- */
    { &KEY_NSPIRE_TAB,        '\t', '\t' },

    /* --- Dedicated shortcuts (math keys repurposed for gameplay) ---
     *
     *  CAT  (catalog)  → RETURN_KEY '\012'
     *       Works as an extra "cursor mode / confirm" key; equivalent
     *       to pressing Enter.  Handy for entering autotravel mode and
     *       confirming targets with one thumb without leaving the
     *       touchpad.  In-game: Enter on dungeon view → showCursor() →
     *       arrows move cursor → CAT/Enter confirm → travel().
     *
     *  TRIG (trig)     → EXPLORE_KEY 'x'
     *       Auto-explores the level in one press.
     */
    { &KEY_NSPIRE_CAT,        '\012', '\012' },
    { &KEY_NSPIRE_TRIG,       'x',    'x'    },

    { NULL, 0, 0 }   /* sentinel */
};

/* -----------------------------------------------------------------------
 * Previous press state table (for edge detection)
 * -------------------------------------------------------------------- */
#define MAX_KEYS  72
static struct {
    const t_key *k;
    bool         was_down;
} s_prev[MAX_KEYS];
static int s_prev_count = 0;

/* State for the 6 special keys (ESC, RET, ENTER, DEL, HOME, MENU) */
static bool s_prev_special[8];

/** Return true if key k was NOT pressed last scan but IS now (rising edge). */
static bool key_just_pressed(const t_key *k)
{
    for (int i = 0; i < s_prev_count; i++) {
        if (s_prev[i].k == k) {
            bool now  = isKeyPressed(*k);
            bool edge = (!s_prev[i].was_down && now);
            s_prev[i].was_down = now;
            return edge;
        }
    }
    /* First time we see this key — register it. */
    if (s_prev_count < MAX_KEYS) {
        bool now = isKeyPressed(*k);
        s_prev[s_prev_count].k        = k;
        s_prev[s_prev_count].was_down = now;
        s_prev_count++;
        return false;
    }
    return false;
}

/* -----------------------------------------------------------------------
 * Main scan function
 * -------------------------------------------------------------------- */
void nspire_input_scan(void)
{
    s_tick += SCAN_INTERVAL_MS;

    /* --- Read modifier state ---------------------------------------- */
    uint8_t hw_mods = 0;
    if (isKeyPressed(KEY_NSPIRE_SHIFT)) hw_mods |= HW_MOD_SHIFT;
    if (isKeyPressed(KEY_NSPIRE_CTRL))  hw_mods |= HW_MOD_CTRL;

    boolean shift_held = (hw_mods & HW_MOD_SHIFT) ? true : false;
    boolean ctrl_held  = (hw_mods & HW_MOD_CTRL)  ? true : false;

    /* --- Special keys (ESC, ENTER, DEL, etc.) ----------------------- */
    {
        const t_key *sp_keys[] = {
            &KEY_NSPIRE_ESC, &KEY_NSPIRE_RET, &KEY_NSPIRE_ENTER,
            &KEY_NSPIRE_DEL, &KEY_NSPIRE_HOME, &KEY_NSPIRE_MENU
        };
        /* BrogueCE codes for these specials.
         * RETURN_KEY = '\012' (0x0A).  Both RET (small enter) and ENTER
         * (big enter) send RETURN_KEY so the player can:
         *   - Enter cursor/autotravel mode (showCursor) in the dungeon
         *   - Confirm a cursor target (travel / interact)
         *   - Confirm selections in menus and inventory             */
        const signed long sp_codes[] = {
            ESCAPE_KEY, '\012', '\012',
            DELETE_KEY, ESCAPE_KEY, ESCAPE_KEY   /* HOME/MENU → ESC */
        };
        const int SP_COUNT = 6;

        for (int i = 0; i < SP_COUNT; i++) {
            bool now  = isKeyPressed(*sp_keys[i]);
            bool edge = (!s_prev_special[i] && now);
            s_prev_special[i] = now;
            if (edge) {
                nspire_event_put_key(sp_codes[i], false, false);
                s_repeat.key        = sp_keys[i];
                s_repeat.code       = sp_codes[i];
                s_repeat.ctrl       = false;
                s_repeat.shift      = false;
                s_repeat.press_tick = s_tick;
                s_repeat.last_tick  = s_tick;
                s_repeat.repeating  = false;
            }
        }
    }

    /* --- Regular key table ------------------------------------------ */
    for (const key_entry_t *e = s_key_table; e->nspire_key != NULL; e++) {
        bool edge = key_just_pressed(e->nspire_key);
        if (!edge) continue;

        signed long raw  = shift_held ? e->shifted : e->unshifted;
        signed long code = raw;
        boolean ev_ctrl  = false;
        boolean ev_shift = shift_held && (raw >= 'A' && raw <= 'Z');

        /* CTRL + letter → control character (Ctrl-A = 1, etc.) */
        if (ctrl_held && raw >= 'a' && raw <= 'z') {
            code = raw - 'a' + 1;
            ev_ctrl  = true;
            ev_shift = false;
        } else if (ctrl_held && raw >= 'A' && raw <= 'Z') {
            code = raw - 'A' + 1;
            ev_ctrl  = true;
            ev_shift = false;
        } else if (ctrl_held) {
            /* Pass ctrl flag for non-letter keys (Brogue uses ctrl+[ etc.) */
            ev_ctrl = true;
        }

        nspire_event_put_key(code, ev_ctrl, ev_shift);

        s_repeat.key        = e->nspire_key;
        s_repeat.code       = code;
        s_repeat.ctrl       = ev_ctrl;
        s_repeat.shift      = ev_shift;
        s_repeat.press_tick = s_tick;
        s_repeat.last_tick  = s_tick;
        s_repeat.repeating  = false;
    }

    /* --- Key repeat ------------------------------------------------- */
    if (s_repeat.key != NULL && isKeyPressed(*s_repeat.key)) {
        uint32_t held_ms  = s_tick - s_repeat.press_tick;
        uint32_t since_ms = s_tick - s_repeat.last_tick;

        if (!s_repeat.repeating) {
            if (held_ms >= NSPIRE_KEY_REPEAT_DELAY) {
                s_repeat.repeating = true;
                s_repeat.last_tick = s_tick;
                nspire_event_put_key(s_repeat.code, s_repeat.ctrl, s_repeat.shift);
            }
        } else if (since_ms >= NSPIRE_KEY_REPEAT_RATE) {
            s_repeat.last_tick = s_tick;
            nspire_event_put_key(s_repeat.code, s_repeat.ctrl, s_repeat.shift);
        }
    } else if (s_repeat.key != NULL && !isKeyPressed(*s_repeat.key)) {
        s_repeat.key = NULL;
    }
}
