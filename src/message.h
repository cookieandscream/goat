#ifndef GOAT_MESSAGE_H
#define GOAT_MESSAGE_H

typedef struct goat_message_tags {
    size_t m_len;
    char m_bytes[512];
} MessageTags;

struct goat_message {
    MessageTags *m_tags;
    GoatCommand m_command;
    int m_have_recognised_command;
    const char *m_prefix;
    const char *m_command_string;
    const char *m_params[16];
    size_t m_len;
    char m_bytes[512];
}; /* typedef'd as GoatMessage in goat.h */

#define GOAT_MESSAGE_MAX_LEN  (510)
#define GOAT_MESSAGE_MAX_TAGS (510)

#endif
