#ifndef GOAT_IRC_H
#define GOAT_IRC_H

typedef struct {
    GoatEvent primary;
    GoatEvent secondary;
} EventPair;

extern const char *const irc_strings[];

extern const EventPair irc_events[];

#endif
