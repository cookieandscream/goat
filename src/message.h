#ifndef GOAT_MESSAGE_H
#define GOAT_MESSAGE_H

#include <stdlib.h>

extern const char *const message_commands[];

goat_message_t *goat_message_new(const char *prefix, const char *command, const char **params);
goat_message_t *goat_message_new_from_string(const char *str, size_t len);
goat_message_t *goat_message_clone(const goat_message_t *orig);

void goat_message_delete(goat_message_t *message);

char *goat_message_strdup(const goat_message_t *message);
const char *const goat_message_static_command(const char *command);

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
