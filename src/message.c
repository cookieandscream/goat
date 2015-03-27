#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "goat.h"

#include "commands.h"

typedef struct s_goat_message_tags {
    size_t m_len;
    char m_bytes[516];
} goat_message_tags_t;

struct s_goat_message {
    goat_message_tags_t *m_tags;
    char *m_prefix;
    char *m_command;
    char *m_params[16];
    size_t m_len;
    char m_bytes[516];
};

goat_message_t *goat_message_new(const char *prefix, const char *command, const char **params) {
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
    if (len > 510)  return NULL;

    goat_message_t *message = calloc(1, sizeof(goat_message_t));
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
    message->m_command = (char *) goat_message_static_command(message->m_command);

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
}

goat_message_t *goat_message_new_from_string(const char *str, size_t len) {
    assert(str != NULL);
    assert(len > 0);
    assert(len == strnlen(str, len + 5));

    // chomp crlf
    if (str[len - 1] == '\x0a') {
        -- len;
        if (str[len - 1] == '\x0d')  -- len;
    }

    goat_message_t *message = calloc(1, sizeof(goat_message_t));
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
    message->m_command = (char *) goat_message_static_command(token);

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

goat_message_t *goat_message_clone(const goat_message_t *orig) {
    assert(orig != NULL);

    goat_message_t *clone = calloc(1, sizeof(goat_message_t));
    if (NULL == clone) return NULL;

    memcpy(clone->m_bytes, orig->m_bytes, orig->m_len);
    clone->m_len = orig->m_len;

    clone->m_prefix = clone->m_bytes + (orig->m_prefix - orig->m_bytes);

    if (orig->m_command >= orig->m_bytes && orig->m_command < orig->m_bytes + orig->m_len) {
        clone->m_command = clone->m_bytes + (orig->m_command - orig->m_bytes);
    }
    else {
        clone->m_command = (char *) goat_message_static_command(orig->m_command);
    }

    for (int i = 0; i < 16; i++) {
        if (orig->m_params[i]) {
            clone->m_params[i] = clone->m_bytes + (orig->m_params[i] - orig->m_bytes);
        }
    }

    return clone;
}

void goat_message_delete(goat_message_t *message) {
    if (message->m_tags) free(message->m_tags);
    free(message);
}

char *goat_message_strdup(const goat_message_t *message) {
    assert(message != NULL);

    size_t len = message->m_len + 1;
    char *str = malloc(len);
    if (str == NULL)  return NULL;

    if (goat_message_cstring(message, str, &len)) {
        return str;
    }

    free(str);
    return NULL;
}

char *goat_message_cstring(const goat_message_t *message, char *buf, size_t *len) {
    assert(message != NULL);
    assert(buf != NULL);
    assert(*len > message->m_len);
    assert(*len >= GOAT_MESSAGE_BUF_SZ);

    if (*len > message->m_len) {
        memcpy(buf, message->m_bytes, message->m_len);
        memset(buf + message->m_len, 0, *len - message->m_len);
        *len = message->m_len;

        for (unsigned i = 0; i < message->m_len; i++) {
            if (buf[i] == '\0') buf[i] = ' ';
        }

        return buf;
    }

    return NULL;
}

const char *goat_message_get_prefix(const goat_message_t *message) {
    assert(message != NULL);
    return message->m_prefix;
}

const char *goat_message_get_command(const goat_message_t *message) {
    assert(message != NULL);
    return message->m_command;
}

const char *goat_message_get_param(const goat_message_t *message, size_t index) {
    assert(message != NULL);
    assert(index <= 16);  // FIXME

    return message->m_params[index];
}

size_t goat_message_get_nparams(const goat_message_t *message) {
    size_t i;

    for (i = 0; i <= 16 && message->m_params[i]; i++)
        ;

    return i;
}
