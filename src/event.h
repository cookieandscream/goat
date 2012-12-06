#ifndef GOAT_EVENT_H
#define GOAT_EVENT_H

#include <config.h>

#include "goat.h"
#include "message.h"

int event_process(goat_context_t *context, int connection, const goat_message_t *message);

#endif
