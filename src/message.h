#ifndef GOAT_MESSAGE_H
#define GOAT_MESSAGE_H

#include <stdlib.h>

typedef struct s_goat_message {
    char *m_prefix;
    char *m_command;
    char *m_params[16];
    size_t m_len;
    char m_bytes[0];
} goat_message_t;

goat_message_t *message_new(const char *str, size_t len);
int message_delete(goat_message_t *message);

#endif
