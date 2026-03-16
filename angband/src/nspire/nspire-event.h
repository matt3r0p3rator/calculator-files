/**
 * \file nspire-event.h
 * \brief Simple ring-buffer event queue for the TI-Nspire port of Angband
 *
 * The event system mirrors the design used by the NDS port
 * (angband/src/nds/nds-event.h).
 */

#ifndef NSPIRE_EVENT_H
#define NSPIRE_EVENT_H

#include "../h-basic.h"
#include "../ui-event.h"

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Event types
 * -------------------------------------------------------------------- */
typedef enum {
    NSPIRE_EVENT_INVALID  = 0,
    NSPIRE_EVENT_KEYBOARD,
} nspire_event_type_t;

/* -----------------------------------------------------------------------
 * Event payloads
 * -------------------------------------------------------------------- */
typedef struct {
    keycode_t key;   /**< Angband key code                              */
    uint8_t   mods;  /**< Modifier flags (KC_MOD_SHIFT / KC_MOD_CTRL …) */
} nspire_event_keyboard_t;

/* -----------------------------------------------------------------------
 * Generic event
 * -------------------------------------------------------------------- */
typedef struct {
    nspire_event_type_t type;
    union {
        nspire_event_keyboard_t keyboard;
    };
} nspire_event_t;

/* -----------------------------------------------------------------------
 * Queue API
 * -------------------------------------------------------------------- */

/** Initialise the event queue.  Returns false on allocation failure. */
bool nspire_event_init(void);

/** Return true when at least one event is waiting in the queue. */
bool nspire_event_ready(void);

/**
 * Dequeue and return the next event.
 * Returns an event with type==NSPIRE_EVENT_INVALID when the queue is empty.
 */
nspire_event_t nspire_event_get(void);

/** Enqueue a keyboard event. */
void nspire_event_put_key(keycode_t key, uint8_t mods);

#endif /* NSPIRE_EVENT_H */
