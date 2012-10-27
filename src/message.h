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

extern const char *const message_commands[];

goat_message_t *message_new(const char *prefix, const char *command, const char **params);
goat_message_t *message_new_from_string(const char *str, size_t len);
int message_delete(goat_message_t *message);

char *message_strdup(const goat_message_t *message);

#endif
