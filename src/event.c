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

    // FIXME if there's a callback for it, call that

    // FIXME or if there's a default callback for it, call that

    // FIXME or if there's a generic callback, call that

    // FIXME or if there's a default generic callback, call that

    return -1; // FIXME
}

goat_event_t _event_get_type(const goat_message_t *message) {
    return GOAT_EVENT_GENERIC; // FIXME
}
