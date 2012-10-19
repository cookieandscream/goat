#ifndef GOAT_EVENT_H
#define GOAT_EVENT_H

#include <config.h>

#include "goat.h"

int event_process(goat_context_t *context, int connection, const char *message);

#endif
