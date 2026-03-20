/**
 * \file nspire/nspire-event.c
 * \brief Ring-buffer event queue for the TI-Nspire port of BrogueCE
 */

#include "nspire-event.h"

#include <stdlib.h>
#include <string.h>

#define NSPIRE_EVENT_BUF_LEN  128

/* Each slot has a valid flag separate from eventType since rogueEvent
 * doesn't have a guaranteed "empty" sentinel value for eventType.      */
typedef struct {
    bool       valid;
    rogueEvent ev;
} slot_t;

static slot_t  *s_buf   = NULL;
static uint16_t s_read  = 0;
static uint16_t s_write = 0;

bool nspire_event_init(void)
{
    s_buf = (slot_t *)malloc(sizeof(slot_t) * NSPIRE_EVENT_BUF_LEN);
    if (!s_buf)
        return false;
    memset(s_buf, 0, sizeof(slot_t) * NSPIRE_EVENT_BUF_LEN);
    s_read  = 0;
    s_write = 0;
    return true;
}

bool nspire_event_ready(void)
{
    return (s_buf != NULL && s_buf[s_read].valid);
}

rogueEvent nspire_event_get(void)
{
    static const rogueEvent empty = { .eventType = -1 };

    if (!nspire_event_ready())
        return empty;

    rogueEvent ev    = s_buf[s_read].ev;
    s_buf[s_read].valid = false;

    if (++s_read >= NSPIRE_EVENT_BUF_LEN)
        s_read = 0;

    return ev;
}

void nspire_event_put_key(signed long key, boolean controlKey, boolean shiftKey)
{
    if (!s_buf)
        return;

    /* Drop event if the queue is full (write is about to lap read). */
    uint16_t next_write = s_write + 1;
    if (next_write >= NSPIRE_EVENT_BUF_LEN) next_write = 0;
    if (next_write == s_read) return;  /* full */

    s_buf[s_write].valid            = true;
    s_buf[s_write].ev.eventType     = KEYSTROKE;
    s_buf[s_write].ev.param1        = key;
    s_buf[s_write].ev.param2        = 0;
    s_buf[s_write].ev.controlKey    = controlKey;
    s_buf[s_write].ev.shiftKey      = shiftKey;

    s_write = next_write;
}

void nspire_event_put_mouse(int eventType, int x, int y)
{
    if (!s_buf)
        return;

    uint16_t next_write = s_write + 1;
    if (next_write >= NSPIRE_EVENT_BUF_LEN) next_write = 0;
    if (next_write == s_read) return;  /* full */

    s_buf[s_write].valid            = true;
    s_buf[s_write].ev.eventType     = eventType;
    s_buf[s_write].ev.param1        = x;
    s_buf[s_write].ev.param2        = y;
    s_buf[s_write].ev.controlKey    = false;
    s_buf[s_write].ev.shiftKey      = false;

    s_write = next_write;
}
