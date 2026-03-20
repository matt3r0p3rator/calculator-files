/**
 * \file nspire/nspire-event.h
 * \brief Simple ring-buffer event queue for the TI-Nspire port of BrogueCE
 *
 * Stores rogueEvent structs so that nspire-input.c can push key events
 * and nspire-platform.c can dequeue them in pauseForMilliseconds /
 * nextKeyOrMouseEvent.
 */

#ifndef NSPIRE_EVENT_H
#define NSPIRE_EVENT_H

#include "Rogue.h"

#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Queue API
 * -------------------------------------------------------------------- */

/** Initialise the event queue.  Returns false on allocation failure. */
bool nspire_event_init(void);

/** Return true when at least one event is waiting in the queue. */
bool nspire_event_ready(void);

/**
 * Dequeue and return the next event.
 * Returns an event with eventType == -1 when the queue is empty.
 */
rogueEvent nspire_event_get(void);

/** Enqueue a keyboard event. */
void nspire_event_put_key(signed long key, boolean controlKey, boolean shiftKey);

/**
 * Enqueue a mouse event.
 * @param eventType  MOUSE_UP, MOUSE_DOWN, MOUSE_ENTERED_CELL, etc.
 * @param x          Window column (terminal x, 0-based)
 * @param y          Window row    (terminal y, 0-based)
 */
void nspire_event_put_mouse(int eventType, int x, int y);

#endif /* NSPIRE_EVENT_H */

