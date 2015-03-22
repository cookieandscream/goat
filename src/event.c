#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "event.h"
#include "message.h"

static const goat_callback_t event_default_callbacks[GOAT_EVENT_LAST] = {

};

static goat_event_t _event_get_type(const goat_message_t *message);

int event_process(goat_context_t *context, int connection, const goat_message_t *message) {
    assert(context != NULL);
    assert(message != NULL);

    goat_event_t event = _event_get_type(message);

    goat_callback_t callback = NULL;

    if (NULL != context->m_callbacks[event]) {
        callback = context->m_callbacks[event];
    }
    else if (NULL != event_default_callbacks[event]) {
        callback = event_default_callbacks[event];
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

    // FIXME how to handle passing params around? length? null-ended?
    callback(context, connection, message->m_prefix, message->m_command, message->m_params);

    return 0;
}

goat_event_t _event_get_type(const goat_message_t *message) {
    return GOAT_EVENT_GENERIC; // FIXME
}
