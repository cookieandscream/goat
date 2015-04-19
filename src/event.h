#ifndef GOAT_EVENT_H
#define GOAT_EVENT_H

#include <config.h>

#include "goat.h"
#include "message.h"

void event_process(GoatContext *context, int connection, const GoatMessage *message);

#endif
