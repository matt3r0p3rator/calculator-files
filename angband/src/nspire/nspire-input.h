/**
 * \file nspire-input.h
 * \brief Keyboard polling for the TI-Nspire CX II (Ndless)
 *
 * Call nspire_input_scan() once per logical "frame" (i.e. every time
 * Angband asks for events).  It polls the hardware keyboard, translates
 * pressed keys into Angband key codes and pushes them onto the event
 * queue via nspire_event_put_key().
 *
 * Key-repeat is handled here: a key that is held down will fire an
 * initial event and then repeat after NSPIRE_KEY_REPEAT_DELAY ms at
 * NSPIRE_KEY_REPEAT_RATE ms intervals.
 */

#ifndef NSPIRE_INPUT_H
#define NSPIRE_INPUT_H

#include <stdbool.h>

/* Milliseconds before a held key begins to repeat. */
#define NSPIRE_KEY_REPEAT_DELAY   300

/* Milliseconds between repeated key events while key is held. */
#define NSPIRE_KEY_REPEAT_RATE    60

/**
 * Scan the hardware keyboard once and push any new key events onto the
 * event queue.  Should be called once per game loop iteration.
 */
void nspire_input_scan(void);

#endif /* NSPIRE_INPUT_H */
