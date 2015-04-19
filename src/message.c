#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goat.h"

#include "irc.h"
#include "message.h"
#include "tags.h"
#include "util.h"

GoatMessage *goat_message_new(const char *prefix, const char *command, const char **params) {
    assert(command != NULL);
    size_t len = 0, n_params = 0;

    if (prefix != NULL) {
        if (str_has_crlf(prefix)) return NULL;
        if (str_has_sp(prefix)) return NULL;
        len += strlen(prefix) + 2;
    }

    if (str_has_crlf(command)) return NULL;
    if (str_has_sp(command)) return NULL;
    len += strlen(command);

    if (params) {
        int have_space_param = 0;

        for (const char **p = params; *p; p++) {
            // a param may not start with :
            if (**p == ':') return NULL;

            if (str_has_crlf(*p)) return NULL;

            // further parameters after one containing a space are invalid
            if (have_space_param) return NULL;
            if (str_has_sp(*p)) have_space_param = 1;

            len += strlen(*p) + 1;
            ++ n_params;
            if (n_params == 15)  break;
        }
        len += 1;
    }
    if (len > GOAT_MESSAGE_MAX_LEN)  return NULL;

    GoatMessage *message = calloc(1, sizeof(GoatMessage));
    if (message == NULL)  return NULL;

    char *position = message->m_bytes;

    if (prefix) {
        *position++ = ':';
        message->m_prefix = position;
        position = stpcpy(position, prefix);
        ++ position;
    }

    if (0 == goat_command(command, &message->m_command)) {
        message->m_have_recognised_command = 1;
        message->m_command_string = goat_command_string(message->m_command);
    }
    else {
        message->m_command_string = position;
    }
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
}

GoatMessage *goat_message_new_from_string(const char *str, size_t len) {
    assert(str != NULL);
    assert(len > 0);
    assert(len == strnlen(str, len + 5));

    // chomp crlf
    if (str[len - 1] == '\x0a') {
        -- len;
        if (str[len - 1] == '\x0d')  -- len;
    }

    GoatMessage *message = calloc(1, sizeof(GoatMessage));
    if (message == NULL)  return NULL;

    // [ '@' tags SPACE ]
    if (str[0] == '@') {
        size_t consumed = tags_parse(str, &message->m_tags);
        if (consumed < 2) goto cleanup; // at least @ and space

        len -= consumed;
        str += consumed;
    }

    if (len > GOAT_MESSAGE_MAX_LEN) goto cleanup;

    message->m_len = len;
    strncpy(message->m_bytes, str, len);

    if (str_has_crlf(message->m_bytes)) goto cleanup;

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
    if (0 == goat_command(token, &message->m_command)) {
        message->m_have_recognised_command = 1;
        message->m_command_string = goat_command_string(message->m_command);
    }
    else {
        message->m_command_string = token;
    }

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

GoatMessage *goat_message_clone(const GoatMessage *orig) {
    assert(orig != NULL);

    GoatMessage *clone = calloc(1, sizeof(GoatMessage));
    if (NULL == clone) return NULL;

    if (NULL != orig->m_tags) {
        clone->m_tags = calloc(1, sizeof(MessageTags));
        if (NULL == clone->m_tags) goto cleanup;

        memcpy(clone->m_tags, orig->m_tags, sizeof(MessageTags));
    }

    memcpy(clone->m_bytes, orig->m_bytes, orig->m_len);
    clone->m_len = orig->m_len;

    if (orig->m_prefix) {
        clone->m_prefix = clone->m_bytes + (orig->m_prefix - orig->m_bytes);
    }

    if (orig->m_have_recognised_command) {
        clone->m_have_recognised_command = orig->m_have_recognised_command;
        clone->m_command = orig->m_command;
        clone->m_command_string = goat_command_string(clone->m_command);
        assert(clone->m_command_string == orig->m_command_string);
    }
    else {
        clone->m_command_string = clone->m_bytes + (orig->m_command_string - orig->m_bytes);
    }

    for (int i = 0; i < 16; i++) {
        if (orig->m_params[i]) {
            clone->m_params[i] = clone->m_bytes + (orig->m_params[i] - orig->m_bytes);
        }
    }

    return clone;

cleanup:
    if (clone && clone->m_tags) free(clone->m_tags);
    if (clone) free(clone);
    return NULL;
}

void goat_message_delete(GoatMessage *message) {
    if (message->m_tags) free(message->m_tags);
    free(message);
}

char *goat_message_strdup(const GoatMessage *message) {
    if (NULL == message) return NULL;

    size_t len = message->m_len + 1;
    char *str = malloc(len);
    if (str == NULL)  return NULL;

    if (goat_message_cstring(message, str, &len)) {
        return str;
    }

    free(str);
    return NULL;
}

char *goat_message_cstring(const GoatMessage *message, char *buf, size_t *len) {
    if (NULL == message) return NULL;
    if (NULL == buf) return NULL;
    if (NULL == len) return NULL;

    size_t min_len = message->m_len;
    if (message->m_tags) {
        min_len += 2 + message->m_tags->m_len; // at, space
    }

    if (*len > min_len) {
        char *p = buf;

        memset(p, 0, *len);

        if (message->m_tags) {
            *p++ = '@';
            p = stpncpy(p, message->m_tags->m_bytes, message->m_tags->m_len);
            *p++ = ' ';
        }

        // embedded nulls, so use memcpy rather than str funcs
        memcpy(p, message->m_bytes, message->m_len);
        p += message->m_len;
        *len = p - buf;

        // replace embedded nulls with spaces
        for (size_t i = 0; i < *len; i++) {
            if (buf[i] == '\0') buf[i] = ' ';
        }

        return buf;
    }

    return NULL;
}

const char *goat_message_get_prefix(const GoatMessage *message) {
    if (NULL == message) return NULL;

    return message->m_prefix;
}

const char *goat_message_get_command_string(const GoatMessage *message) {
    if (NULL == message) return NULL;

    return message->m_command_string;
}

const char *goat_message_get_param(const GoatMessage *message, size_t index) {
    if (NULL == message) return NULL;
    if (index >= 16) return NULL;

    return message->m_params[index];
}

size_t goat_message_get_nparams(const GoatMessage *message) {
    if (NULL == message) return 0;

    size_t i;

    for (i = 0; i < 16 && message->m_params[i]; i++)
        ;

    return i;
}

int goat_message_get_command(const GoatMessage *message, GoatCommand *command) {
    if (NULL == message) return -1;
    if (NULL == command) return -1;

    if (message->m_have_recognised_command) {
        *command = message->m_command;
        return 0;
    }

    return -1;
}
