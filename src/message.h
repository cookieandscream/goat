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

const char *message_static_command(const char *command);

///

// FIXME these structs will need to be private
struct s_kv_list_node {
    char *key;
    char *value;
    struct s_kv_list_node *prev;
    struct s_kv_list_node *next;
};

struct s_goat_message {
    struct s_kv_list_node *m_tags;
    char *m_prefix;
    char *m_command;
    char *m_params[16];
};

typedef struct s_goat_message goat_message_t;

goat_message_t *goat_message_new(const char *prefix, const char *command, const char **params);
goat_message_t *goat_message_new_from_string(const char *str, size_t len);
goat_message_t *goat_message_clone(const goat_message_t *orig);
void goat_message_delete(goat_message_t *message);

int goat_message_set_prefix(goat_message_t *message, const char *prefix);
int goat_message_set_command(goat_message_t *message, const char *command);
int goat_message_add_params(goat_message_t *message, ...);

int goat_message_get_prefix(const goat_message_t *message, char *prefix, size_t *size);
int goat_message_get_command(const goat_message_t *message, char *command, size_t *size);
// FIXME api for getting params
size_t goat_message_get_nparams(const goat_message_t *message);
int goat_message_get_param(const goat_message_t *message, int index, char *param, size_t *size);

int goat_message_set_tag(goat_message_t *message, const char *key, const char *value);
size_t goat_message_has_tags(const goat_message_t *message);
int goat_message_has_tag(const goat_message_t *message, const char *key);
int goat_message_get_tag(const goat_message_t *message, const char *key, char *value, size_t *size);
int goat_message_unset_tag(goat_message_t *message, const char *key);


#endif
