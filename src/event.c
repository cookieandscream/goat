#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "event.h"
#include "irc.h"
#include "message.h"

static const goat_callback_t event_default_callbacks[GOAT_EVENT_LAST] = {

};

static const goat_event_pair_t *_event_get_type(const goat_message_t *message);

int event_process(goat_context_t *context, int connection, const goat_message_t *message) {
    assert(context != NULL);
    assert(message != NULL);

    const goat_event_pair_t *e = _event_get_type(message);

    goat_callback_t callback = NULL;

    if (NULL != context->m_callbacks[e->primary]) {
        callback = context->m_callbacks[e->primary];
    }
    else if (NULL != event_default_callbacks[e->primary]) {
        callback = event_default_callbacks[e->primary];
    }
    else if (NULL != context->m_callbacks[e->secondary]) {
        callback = context->m_callbacks[e->secondary];
    }
    else if (NULL != event_default_callbacks[e->secondary]) {
        callback = event_default_callbacks[e->secondary];
    }
    else if (NULL != context->m_callbacks[GOAT_EVENT_GENERIC]) {
        callback = context->m_callbacks[GOAT_EVENT_GENERIC];
    }
    else if (NULL != event_default_callbacks[GOAT_EVENT_GENERIC]) {
        callback = event_default_callbacks[GOAT_EVENT_GENERIC];
    }
    else {
        return 0;
    }

    callback(context, connection, message);

    return 0;
}

const goat_event_pair_t *_event_get_type(const goat_message_t *message) {
    return &irc_events[0]; // FIXME
}
