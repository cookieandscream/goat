#include <assert.h>
#include <string.h>

#include "message.h"

goat_message_t *message_new(const char *prefix, const char *command, const char **params) {
    assert(command != NULL);
    size_t len = 0, n_params = 0;

    if (prefix != NULL)  len += strlen(prefix) + 2;
    len += strlen(command);
    if (params) {
        for (const char **p = params; *p; p++) {
            len += strlen(*p) + 1;
            ++ n_params;
            if (n_params == 15)  break;
            if (strchr(*p, ' ') != NULL)  break;
        }
        len += 1;
    }

    goat_message_t *message = calloc(1, sizeof(goat_message_t) + len + 1);
    if (message == NULL)  return NULL;

    char *position = message->m_bytes;

    if (prefix) {
        *position++ = ':';
        message->m_prefix = position;
        position = stpcpy(position, prefix);
        ++ position;
    }

    message->m_command = position;
    position = stpcpy(position, command);

    if (params) {
        size_t i;
        for (i = 0; i < n_params - 1; i++) {
            ++position;
            message->m_params[i] = position;
            position = stpcpy(position, params[i]);
        }
        ++position;
        *position++ = ':';
        message->m_params[i] = position;
        position = stpcpy(position, params[i]);
    }

    message->m_len = position - message->m_bytes;
    return message;

cleanup:
    free(message);
    return NULL;
}

goat_message_t *message_new_from_string(const char *str, size_t len) {
    assert(str != NULL);
    assert(len > 0);
    assert(len == strnlen(str, len + 5));

    // chomp crlf
    if (str[len - 1] == '\x0a') {
        -- len;
        if (str[len - 1] == '\x0d')  -- len;
    }

    goat_message_t *message = calloc(1, sizeof(goat_message_t) + len + 1);
    if (message == NULL)  return NULL;

    message->m_len = len;
    strncpy(message->m_bytes, str, len);

    char *position = message->m_bytes;
    char *token;

    // [ ':' prefix SPACE ]
    if (position[0] == ':') {
        ++ position;
        token = strsep(&position, " ");
        if (token[0] == '\0')  goto cleanup;
        message->m_prefix = token;
    }

    // command
    token = strsep(&position, " ");
    if (token == NULL || token[0] == '\0')  goto cleanup;
    message->m_command = token;

    // *14( SPACE middle ) [ SPACE ":" trailing ]
    // 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
    unsigned i = 0;
    while (i < 14 && position) {
        if (position[0] == ':')  break;
        token = strsep(&position, " ");
        message->m_params[i] = token;
        ++ i;
    }
    if (position && position[0] == ':')  ++position;
    message->m_params[i] = position;

    return message;

cleanup:
    free(message);
    return NULL;
}

int message_delete(goat_message_t *message) {
    free(message);
    return 0;
}

char *message_strdup(const goat_message_t *message) {
    assert(message != NULL);

    char *str = malloc(message->m_len + 1);
    if (str == NULL)  return NULL;

    memcpy(str, message->m_bytes, message->m_len);
    str[message->m_len] = '\0';

    for (unsigned i = 0; i < message->m_len; i++) {
        if (str[i] == '\0')  str[i] = ' ';
    }

    return str;
}
