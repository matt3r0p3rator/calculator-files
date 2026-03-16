/**
 * \file nspire-input.c
 * \brief Keyboard polling and key mapping for the TI-Nspire CX II
 *
 * The TI-Nspire CX II has a 80-key keyboard layout.  This file maps
 * hardware keys to the ASCII / Angband key codes expected by the game.
 *
 * Modifier handling
 * -----------------
 * - SHIFT          : produces upper-case letters / shifted symbols
 * - CTRL           : sets KC_MOD_CTRL flag; ctrl+<letter> produces
 *                    the corresponding control character (0x01–0x1A)
 * - The Nspire has no dedicated ALT key; we ignore that modifier.
 *
 * Movement mapping (numpad roguelike convention)
 * -----------------------------------------------
 *   Up      → '8'     Down     → '2'
 *   Left    → '4'     Right    → '6'
 *   UpRight → '9'     DownLeft → '1'
 *   UpLeft  → '7'     DownRight→ '3'
 *   Touchpad click (center) → '5' (wait in place)
 *
 * Special keys
 * ------------
 *   ESC    → 0x1B    RET/ENTER → '\r'    DEL  → 0x7F
 *   TAB    → '\t'    SPACE     → ' '
 *   Home   → HOME (TERM_KEY_HOME via keycode)
 *
 * Angband uses the raw numpad for movement, so we map the touchpad
 * arrows to number chars '1'–'9'.
 */

#include "nspire-input.h"
#include "nspire-event.h"

/* Angband key codes and modifier flags */
#include "../ui-event.h"

#include <libndls.h>
#include <keys.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Hardware modifier state (internal names to avoid name clash)
 * -------------------------------------------------------------------- */
#define HW_MOD_SHIFT   0x01
#define HW_MOD_CTRL    0x02

/* -----------------------------------------------------------------------
 * Key repeat state
 * -------------------------------------------------------------------- */
typedef struct {
    const t_key *key;        /**< currently held key (or NULL)        */
    keycode_t    code;       /**< Angband code that was emitted        */
    uint8_t      mods;       /**< modifier flags at press time         */
    uint32_t     press_tick; /**< msleep-relative tick of first press  */
    uint32_t     last_tick;  /**< tick of last repeat emission         */
    bool         repeating;  /**< has initial-delay elapsed?           */
} repeat_state_t;

static repeat_state_t s_repeat = {0};

/* Simple millisecond tick counter (incremented in nspire_input_scan) */
static uint32_t s_tick = 0;
#define SCAN_INTERVAL_MS  10   /* approx. ms per scan call */

/* -----------------------------------------------------------------------
 * Key table entry
 * -------------------------------------------------------------------- */
typedef struct {
    const t_key *nspire_key;   /**< Ndless key struct pointer            */
    keycode_t    unshifted;    /**< keycode when SHIFT is not held        */
    keycode_t    shifted;      /**< keycode when SHIFT is held            */
    bool         is_movement;  /**< true for directional keys             */
} key_entry_t;

/* KC_MOD_CONTROL = 0x01, KC_MOD_SHIFT = 0x02 are from ui-event.h */

/* -----------------------------------------------------------------------
 * Main key table (Nspire CX layout)
 * Characters marked '\0' are handled by special cases below.
 * -------------------------------------------------------------------- */
static const key_entry_t s_key_table[] = {
    /* Letters */
    { &KEY_NSPIRE_A,        'a', 'A', false },
    { &KEY_NSPIRE_B,        'b', 'B', false },
    { &KEY_NSPIRE_C,        'c', 'C', false },
    { &KEY_NSPIRE_D,        'd', 'D', false },
    { &KEY_NSPIRE_E,        'e', 'E', false },
    { &KEY_NSPIRE_F,        'f', 'F', false },
    { &KEY_NSPIRE_G,        'g', 'G', false },
    { &KEY_NSPIRE_H,        'h', 'H', false },
    { &KEY_NSPIRE_I,        'i', 'I', false },
    { &KEY_NSPIRE_J,        'j', 'J', false },
    { &KEY_NSPIRE_K,        'k', 'K', false },
    { &KEY_NSPIRE_L,        'l', 'L', false },
    { &KEY_NSPIRE_M,        'm', 'M', false },
    { &KEY_NSPIRE_N,        'n', 'N', false },
    { &KEY_NSPIRE_O,        'o', 'O', false },
    { &KEY_NSPIRE_P,        'p', 'P', false },
    { &KEY_NSPIRE_Q,        'q', 'Q', false },
    { &KEY_NSPIRE_R,        'r', 'R', false },
    { &KEY_NSPIRE_S,        's', 'S', false },
    { &KEY_NSPIRE_T,        't', 'T', false },
    { &KEY_NSPIRE_U,        'u', 'U', false },
    { &KEY_NSPIRE_V,        'v', 'V', false },
    { &KEY_NSPIRE_W,        'w', 'W', false },
    { &KEY_NSPIRE_X,        'x', 'X', false },
    { &KEY_NSPIRE_Y,        'y', 'Y', false },
    { &KEY_NSPIRE_Z,        'z', 'Z', false },

    /* Digits */
    { &KEY_NSPIRE_0,        '0', ')', false },
    { &KEY_NSPIRE_1,        '1', '!', false },
    { &KEY_NSPIRE_2,        '2', '@', false },
    { &KEY_NSPIRE_3,        '3', '#', false },
    { &KEY_NSPIRE_4,        '4', '$', false },
    { &KEY_NSPIRE_5,        '5', '%', false },
    { &KEY_NSPIRE_6,        '6', '^', false },
    { &KEY_NSPIRE_7,        '7', '&', false },
    { &KEY_NSPIRE_8,        '8', '*', false },
    { &KEY_NSPIRE_9,        '9', '(', false },

    /* Punctuation directly on Nspire CX keyboard */
    { &KEY_NSPIRE_SPACE,       ' ',  ' ',  false },
    { &KEY_NSPIRE_PERIOD,      '.',  '>',  false },
    { &KEY_NSPIRE_COMMA,       ',',  '<',  false },
    { &KEY_NSPIRE_MINUS,       '-',  '_',  false },
    { &KEY_NSPIRE_PLUS,        '+',  '=',  false },
    { &KEY_NSPIRE_LP,          '(',  '[',  false },
    { &KEY_NSPIRE_RP,          ')',  ']',  false },
    { &KEY_NSPIRE_DIVIDE,      '/',  '/',  false }, /* '?' has its own key */
    { &KEY_NSPIRE_MULTIPLY,    '*',  '|',  false },
    { &KEY_NSPIRE_EQU,         '=',  '+',  false },
    { &KEY_NSPIRE_EXP,         '^',  '~',  false }, /* shift+EXP → ~ (only missing ASCII) */
    /* Dedicated punctuation keys (bypass the OS special-key menu) */
    { &KEY_NSPIRE_QUES,        '?',  '?',  false },
    { &KEY_NSPIRE_DOC,         '?',  '?',  false }, /* doc key → ? */
    { &KEY_NSPIRE_COLON,       ':',  ';',  false },
    { &KEY_NSPIRE_QUOTE,       '"',  '\'' ,false },
    { &KEY_NSPIRE_APOSTROPHE,  '\'' ,'`',  false },
    { &KEY_NSPIRE_BAR,         '|',  '\\', false },
    { &KEY_NSPIRE_GTHAN,       '>',  '}',  false },
    { &KEY_NSPIRE_LTHAN,       '<',  '{',  false },
    { &KEY_NSPIRE_NEGATIVE,    '-',  '_',  false },

    /* Arrow keys → Angband ARROW_* codes (work in menus AND movement) */
    { &KEY_NSPIRE_UP,       ARROW_UP,    ARROW_UP,    true  },
    { &KEY_NSPIRE_DOWN,     ARROW_DOWN,  ARROW_DOWN,  true  },
    { &KEY_NSPIRE_LEFT,     ARROW_LEFT,  ARROW_LEFT,  true  },
    { &KEY_NSPIRE_RIGHT,    ARROW_RIGHT, ARROW_RIGHT, true  },

    /* Diagonal touchpad directions → roguelike numpad chars */
    { &KEY_NSPIRE_UPRIGHT,   '9', '9', true },
    { &KEY_NSPIRE_RIGHTDOWN, '3', '3', true },
    { &KEY_NSPIRE_DOWNLEFT,  '1', '1', true },
    { &KEY_NSPIRE_LEFTUP,    '7', '7', true },

    /* Touchpad click = wait in place */
    { &KEY_NSPIRE_CLICK,    '5', '5', true  },

    /* Control/meta */
    { &KEY_NSPIRE_TAB,      '\t', '\t', false },

    { NULL, 0, 0, false }   /* sentinel */
};

/* -----------------------------------------------------------------------
 * Previous press state for edge detection
 * -------------------------------------------------------------------- */
#define MAX_KEYS  64
static struct {
    const t_key *k;
    bool          was_down;
} s_prev[MAX_KEYS];
static int s_prev_count = 0;

static bool s_prev_special[8];  /* ESC, RET, ENTER, DEL, HOME, MENU, ... */

/** Return true if key k was NOT pressed last scan but IS pressed now. */
static bool key_just_pressed(const t_key *k)
{
    for (int i = 0; i < s_prev_count; i++) {
        if (s_prev[i].k == k) {
            bool now = isKeyPressed(*k);
            bool edge = (!s_prev[i].was_down && now);
            s_prev[i].was_down = now;
            return edge;
        }
    }
    /* First time we see this key – register it */
    if (s_prev_count < MAX_KEYS) {
        bool now = isKeyPressed(*k);
        s_prev[s_prev_count].k       = k;
        s_prev[s_prev_count].was_down = now;
        s_prev_count++;
        return false;  /* can't be a "just pressed" edge on first scan */
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

    uint8_t ang_mods = 0;
    if (hw_mods & HW_MOD_SHIFT) ang_mods |= KC_MOD_SHIFT;
    if (hw_mods & HW_MOD_CTRL)  ang_mods |= KC_MOD_CONTROL;

    /* --- Check special keys ----------------------------------------- */
    {
        /* Index: 0=ESC, 1=RET, 2=ENTER, 3=DEL, 4=HOME, 5=MENU */
        const t_key *sp_keys[] = {
            &KEY_NSPIRE_ESC, &KEY_NSPIRE_RET, &KEY_NSPIRE_ENTER,
            &KEY_NSPIRE_DEL, &KEY_NSPIRE_HOME, &KEY_NSPIRE_MENU
        };
        const keycode_t sp_codes[] = {
            0x1B, '\r', '\r', 0x08, KC_HOME, 0x1B
        };
        const int SP_COUNT = 6;

        for (int i = 0; i < SP_COUNT; i++) {
            bool now  = isKeyPressed(*sp_keys[i]);
            bool edge = (!s_prev_special[i] && now);
            s_prev_special[i] = now;
            if (edge) {
                uint8_t m = ang_mods;
                /* Apply CTRL transformation for RET/ENTER */
                nspire_event_put_key(sp_codes[i], m);

                /* Update repeat tracker */
                s_repeat.key        = sp_keys[i];
                s_repeat.code       = sp_codes[i];
                s_repeat.mods       = m;
                s_repeat.press_tick = s_tick;
                s_repeat.last_tick  = s_tick;
                s_repeat.repeating  = false;
            }
        }
    }

    /* --- Scan regular key table ------------------------------------- */
    for (const key_entry_t *e = s_key_table; e->nspire_key != NULL; e++) {
        bool edge = key_just_pressed(e->nspire_key);
        if (edge) {
            keycode_t raw = (hw_mods & HW_MOD_SHIFT) ? e->shifted : e->unshifted;
            keycode_t code = raw;
            uint8_t mods = ang_mods;

            /* CTRL+letter → control character, clear shift flag */
            if ((hw_mods & HW_MOD_CTRL) && raw >= 'a' && raw <= 'z') {
                code = (keycode_t)(raw - 'a' + 1);
                mods &= ~KC_MOD_SHIFT;
            } else if ((hw_mods & HW_MOD_CTRL) && raw >= 'A' && raw <= 'Z') {
                code = (keycode_t)(raw - 'A' + 1);
                mods &= ~KC_MOD_SHIFT;
            }

            nspire_event_put_key(code, mods);

            /* Start repeat tracking */
            s_repeat.key        = e->nspire_key;
            s_repeat.code       = code;
            s_repeat.mods       = mods;
            s_repeat.press_tick = s_tick;
            s_repeat.last_tick  = s_tick;
            s_repeat.repeating  = false;
        }
    }

    /* --- Key repeat ------------------------------------------------- */
    if (s_repeat.key != NULL && isKeyPressed(*s_repeat.key)) {
        uint32_t held_ms  = s_tick - s_repeat.press_tick;
        uint32_t since_ms = s_tick - s_repeat.last_tick;

        if (!s_repeat.repeating) {
            if (held_ms >= NSPIRE_KEY_REPEAT_DELAY) {
                s_repeat.repeating = true;
                s_repeat.last_tick = s_tick;
                nspire_event_put_key(s_repeat.code, s_repeat.mods);
            }
        } else {
            if (since_ms >= NSPIRE_KEY_REPEAT_RATE) {
                s_repeat.last_tick = s_tick;
                nspire_event_put_key(s_repeat.code, s_repeat.mods);
            }
        }
    } else if (s_repeat.key != NULL && !isKeyPressed(*s_repeat.key)) {
        /* Key released – stop repeat */
        s_repeat.key = NULL;
    }
}
