#ifndef GOAT_IRC_H
#define GOAT_IRC_H

typedef struct {
    goat_event_t primary;
    goat_event_t secondary;
} goat_event_pair_t;

extern const char *const irc_strings[];

extern const goat_event_pair_t irc_events[];

#endif
