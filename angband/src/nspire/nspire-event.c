/**
 * \file nspire-event.c
 * \brief Ring-buffer event queue for the TI-Nspire port of Angband
 */

#include "nspire-event.h"

#include <stdlib.h>
#include <string.h>

#define NSPIRE_EVENT_BUF_LEN  128

static nspire_event_t *s_buf   = NULL;
static uint16_t        s_read  = 0;
static uint16_t        s_write = 0;

bool nspire_event_init(void)
{
    s_buf = (nspire_event_t *)malloc(sizeof(nspire_event_t) * NSPIRE_EVENT_BUF_LEN);
    if (!s_buf)
        return false;

    memset(s_buf, 0, sizeof(nspire_event_t) * NSPIRE_EVENT_BUF_LEN);
    s_read  = 0;
    s_write = 0;
    return true;
}

bool nspire_event_ready(void)
{
    return (s_buf && s_buf[s_read].type != NSPIRE_EVENT_INVALID);
}

nspire_event_t nspire_event_get(void)
{
    static const nspire_event_t empty = { .type = NSPIRE_EVENT_INVALID };

    if (!nspire_event_ready())
        return empty;

    nspire_event_t ev       = s_buf[s_read];
    s_buf[s_read].type      = NSPIRE_EVENT_INVALID;

    if (++s_read >= NSPIRE_EVENT_BUF_LEN)
        s_read = 0;

    return ev;
}

void nspire_event_put_key(keycode_t key, uint8_t mods)
{
    if (!s_buf)
        return;

    s_buf[s_write].type              = NSPIRE_EVENT_KEYBOARD;
    s_buf[s_write].keyboard.key      = key;
    s_buf[s_write].keyboard.mods     = mods;

    if (++s_write >= NSPIRE_EVENT_BUF_LEN)
        s_write = 0;
}
