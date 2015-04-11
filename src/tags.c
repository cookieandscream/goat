#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goat.h"

#include "message.h"
#include "tags.h"
#include "util.h"

static const char *_next_tag(const char *str);
static const char *_find_tag(const char *str, const char *key);
static const char *_find_value(const char *str);
static const char *_escape_value(const char *value, char *buf, size_t *size);
static const char *_unescape_value(const char *value, char *buf, size_t *size);

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

    // FIXME make sure buffer is big enough...

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
    if (*end) end--;
    *size = end - v;

    char unescaped[GOAT_MESSAGE_MAX_TAGS];
    size_t unescaped_len = sizeof(unescaped);
    _unescape_value(v, unescaped, &unescaped_len);
    strncpy(value, unescaped, *size);
    // FIXME sort out *size properly

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

    const char *rest = (0 == cmp ? _next_tag(p) : p);
    strcpy(tmp, rest);

    snprintf(kvbuf, sizeof(kvbuf), "%s=%s;", key, escaped_value);

    p = stpcpy(p, kvbuf);
    strcpy(p, tmp);

    return 0;
}

int goat_message_unset_tag(goat_message_t *message, const char *key) {
    assert(message != NULL);
    assert(key != NULL);

    goat_message_tags_t *tags = message->m_tags;

    if (NULL == tags || 0 == strlen(tags->m_bytes)) return 0;

    char *p1, *p2, *end;

    p1 = (char *) _find_tag(tags->m_bytes, key);
    if (!*p1) return 0;

    end = &tags->m_bytes[tags->m_len];

    p2 = (char *) _next_tag(p1);

    if (*p2) {
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

size_t tags_parse(const char *str, goat_message_tags_t **tagsp) {
    if (NULL == tagsp) return 0;
    if (str[0] != '@') return 0;

    // populate our struct, skipping the @
    char *end = strchr(&str[1], ' ');
    size_t len = end - &str[1];

    if (strn_has_crlf(&str[1], len)) return 0;
    if (strn_has_sp(&str[1], len)) return 0;

    if (len > 0 && len <= GOAT_MESSAGE_MAX_TAGS) {
        goat_message_tags_t *tags = calloc(1, sizeof(goat_message_tags_t));
        if (NULL == tags) return 0;

        tags->m_len = len;
        strncpy(tags->m_bytes, &str[1], len);

        *tagsp = tags;
    }

    // calculate consumed length (including @ and space)
    if (*end != '\0') end ++;
    size_t consumed = end - str;

    return consumed;
}

const char *_next_tag(const char *str) {
    assert(str != NULL);

    const char *p = str;

    while (*p != '\0' && *p != ';') p++;

    if (*p == ';') return p + 1;

    return p;
}

const char *_find_tag(const char *str, const char *key) {
    assert(str != NULL);
    assert(key != NULL);

    const char *p = str;
    const size_t key_len = strlen(key);

    while (*p) {
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

const char *_escape_value(const char *value, char *buf, size_t *size) {
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

const char *_unescape_value(const char *value, char *buf, size_t *size) {
    assert(value != NULL);
    assert(buf != NULL);
    assert(size != NULL);

    char *dest = buf;

    size_t value_len = strlen(value);
    assert(value_len > *size);

    for (size_t i = 0; i < value_len; i++) {
        if (value[i] == '\\') {
            char c = value[i + 1];
            switch (c) {
                case ':':  c = ';';          break;
                case 's':  c = ' ';          break;
                case '0':  c = '\0';         break;
                case '\\': c = '\\';         break;
                case 'r':  c = '\r';         break;
                case 'n':  c = '\n';         break;
            }
            *dest++ = c;
            i++;
        }
        else {
            *dest++ = value[i];
        }
    }
    memset(dest, 0, *size - (dest - buf));

    *size = dest - buf;
    return buf;
}
