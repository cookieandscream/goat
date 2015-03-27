#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goat.h"

#include "commands.h"
#include "message.h"

static const char *_next_tag(const char *str);
static const char *_find_tag(const char *str, const char *key);
static const char *_find_value(const char *str);
static char *_escape_value(const char *value, char *buf, size_t *size);

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

size_t goat_message_has_tags(const goat_message_t *message) {
    assert(message != NULL);

    const goat_message_tags_t *tags = message->m_tags;

    if (NULL == tags || 0 == strlen(tags->m_bytes)) return 0;

    size_t count = 0;
    const char *p = tags->m_bytes;
    while (*p != '\0') {
        if (*p == ';') count++;
        p++;
    }

    return count;
}

int goat_message_has_tag(const goat_message_t *message, const char *key) {
    assert(message != NULL);

    const goat_message_tags_t *tags = message->m_tags;

    if (NULL == tags || 0 == strlen(tags->m_bytes)) return 0;

    return _find_tag(tags->m_bytes, key) ? 1 : 0;
}

int goat_message_get_tag_value(
    const goat_message_t *message, const char *key, char *value, size_t *size
) {
    assert(message != NULL);
    assert(key != NULL);
    assert(value != NULL);
    assert(size != NULL);

    const goat_message_tags_t *tags = message->m_tags;
    memset(value, 0, *size);

    if (NULL == tags || 0 == strlen(tags->m_bytes)) return 0;

    const char *p, *v, *end;

    if (NULL == (p = _find_tag(tags->m_bytes, key))) {
        *size = 0;
        return 0;
    }

    if (NULL == (v = _find_value(p))) {
        *size = 0;
        return 0;
    }

    end = _next_tag(v);
    end = (end ? end - 1 : &tags->m_bytes[tags->m_len]);
    *size = end - v;

    // FIXME unescape
    strncpy(value, v, *size);

    return 1;
}

int goat_message_set_tag(goat_message_t *message, const char *key, const char *value) {
    assert(message != NULL);
    assert(key != NULL);

    char escaped_value[GOAT_MESSAGE_MAX_TAGS];
    size_t escaped_value_len = sizeof(escaped_value);

    goat_message_tags_t *tags = message->m_tags;

    size_t tag_len = 1 + strlen(key);
    if (value) {
        _escape_value(value, escaped_value, &escaped_value_len);
        tag_len += 1 + escaped_value_len;
    }

    if (tags->m_len + tag_len > GOAT_MESSAGE_MAX_TAGS) return -1;

    char *p, kvbuf[GOAT_MESSAGE_MAX_TAGS], tmp[GOAT_MESSAGE_MAX_TAGS] = {0};
    const size_t key_len = strlen(key);

    p = tags->m_bytes;
    int cmp;
    while (p && *p) {
        if ((cmp = strncmp(p, key, key_len)) >= 0) break;

        p = (char *) _next_tag(p);
    }

    // FIXME this could probably be tidier
    if (p) {
        if (cmp) {
            // found existing tag, save from the following tag on
            const char *next = _next_tag(p);
            if (next)  strcpy(tmp, next);
            else       p = &tags->m_bytes[tags->m_len];
        }
        else {
            // tag doesn't already exist, just save the rest
            strcpy(tmp, p);
        }
    }
    else {
        p = &tags->m_bytes[tags->m_len];
    }

    snprintf(kvbuf, sizeof(kvbuf), "%s=%s;", key, escaped_value);
    p = stpcpy(p, kvbuf);
    if (tmp[0] != '\0') strcpy(p, tmp);

    return 0;
}

int goat_message_unset_tag(goat_message_t *message, const char *key) {
    assert(message != NULL);
    assert(key != NULL);

    goat_message_tags_t *tags = message->m_tags;

    if (NULL == tags || 0 == strlen(tags->m_bytes)) return 0;

    char *p1, *p2, *end;

    p1 = (char *) _find_tag(tags->m_bytes, key);
    if (NULL == p1) return 0;

    end = &tags->m_bytes[tags->m_len];

    p2 = (char *) _next_tag(p1);

    if (p2) {
        memmove(p1, p2, end - p2);
        tags->m_len -= (p2 - p1);
        memset(&tags->m_bytes[tags->m_len], 0, sizeof(tags->m_bytes) - tags->m_len);
    }
    else {
        tags->m_len = p1 - tags->m_bytes;
        memset(p1, 0, sizeof(tags->m_bytes) - tags->m_len);
    }

    return 1;
}

const char *_next_tag(const char *str) {
    assert(str != NULL);

    const char *p = str;

    while (*p != '\0' && *p != ';') p++;

    if (*p == ';' && *(p + 1) != '\0') return p + 1;

    return NULL;
}

const char *_find_tag(const char *str, const char *key) {
    assert(str != NULL);
    assert(key != NULL);

    const char *p = str;
    const size_t key_len = strlen(key);

    while (p) {
        if (0 == strncmp(p, key, key_len)) {
            switch (*(p + key_len)) {
                case '\0':
                case '=':
                case ';':
                    return p;
            }
        }

        p = _next_tag(p);
    }

    return NULL;
}

const char *_find_value(const char *str) {
    assert(str != NULL);

    const char *p = str;

    while (*p != '\0' && *p != ';' && *p != '=') p++;

    if (*p == '=' && *(p + 1) != '\0') return p + 1;

    return NULL;
}

char *_escape_value(const char *value, char *buf, size_t *size) {
    assert(value != NULL);
    assert(buf != NULL);
    assert(size != NULL);

    char *dest = buf;

    for (size_t i = 0; i < *size; i++) {
        switch(value[i]) {
            case ';':
                dest[0] = '\\';
                dest[1] = ':';
                dest += 2;
                break;

            case ' ':
                dest[0] = '\\';
                dest[1] = 's';
                dest += 2;
                break;

            case '\0':
                dest[0] = '\\';
                dest[1] = '0';
                dest += 2;
                break;

            case '\\':
                // FIXME spec says just single '\', is that a typo?
                // https://github.com/ircv3/ircv3-specifications/blob/master/specification/message-tags-3.2.md
                dest[0] = '\\';
                dest[1] = '\\';
                dest += 2;
                break;

            case '\r':
                dest[0] = '\\';
                dest[1] = 'r';
                dest += 2;
                break;

            case '\n':
                dest[0] = '\\';
                dest[1] = 'n';
                dest += 2;
                break;

            default:
                *dest++ = value[i];
                break;
        }
    }

    *dest = '\0';
    *size = dest - buf;

    return buf;
}
