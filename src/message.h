#ifndef GOAT_MESSAGE_H
#define GOAT_MESSAGE_H

typedef struct s_goat_message_tags {
    size_t m_len;
    char m_bytes[512];
} goat_message_tags_t;

struct s_goat_message {
    goat_message_tags_t *m_tags;
    char *m_prefix;
    char *m_command;
    char *m_params[16];
    size_t m_len;
    char m_bytes[512];
}; /* typedef'd as goat_message_t in goat.h */

#define GOAT_MESSAGE_MAX_LEN  (510)
#define GOAT_MESSAGE_MAX_TAGS (510)

#endif
