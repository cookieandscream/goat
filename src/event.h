#ifndef GOAT_EVENT_H
#define GOAT_EVENT_H

#include <config.h>

#include "goat.h"

extern const goat_callback_t event_default_callbacks[];

int event_process(goat_context_t *context, int connection, const char *message);

#endif
