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

    goat_callback_msg_t cb_msg = { 0 };

    if (message->m_prefix)  cb_msg.prefix = strdup(message->m_prefix);
    if (message->m_command)  cb_msg.command = strdup(message->m_command);

    size_t i;
    for (i = 0; i < 16; i++) {
        if (NULL == message->m_params[i])  break;
        cb_msg.params[i] = strdup(message->m_params[i]);
    }
    cb_msg.nparams = i;

    callback(context, connection, &cb_msg);

    if (cb_msg.prefix)  free(cb_msg.prefix);
    if (cb_msg.command)  free(cb_msg.command);
    for (int i = 0; i < cb_msg.nparams; i++)  free(cb_msg.params[i]);

    return 0;
}

goat_event_t _event_get_type(const goat_message_t *message) {
    return GOAT_EVENT_GENERIC; // FIXME
}
