#ifndef GOAT_EVENT_H
#define GOAT_EVENT_H

#include <config.h>

typedef struct {
    int handle;
    size_t message_len;
    char message[516];
} goat_event_queue_entry;

#define GOAT_EVENT_QUEUE_SIZE (10)

goat_event_queue_entry goat_event_queue[GOAT_EVENT_QUEUE_SIZE];

int event_queue_push(int, size_t, const char *);
int event_queue_pop(/*TODO*/);

#endif
