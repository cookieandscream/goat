#ifndef GOAT_UTIL_H
#define GOAT_UTIL_H

#include <string.h>

#define ARG_UNUSED(expr)         do { (void)(expr); } while (0)

inline int str_has_crlf(const char *str) {
    return strcspn(str, "\x0d\x0a") < strlen(str);
}

inline int str_has_sp(const char *str) {
    return NULL != strchr(str, ' ');
}

inline int strn_has_crlf(const char *str, size_t len) {
    char buf[len + 1];

    strncpy(buf, str, len);
    buf[len] = '\0';

    return str_has_crlf(buf);
}

inline int strn_has_sp(const char *str, size_t len) {
    char buf[len + 1];

    strncpy(buf, str, len);
    buf[len] = '\0';

    return str_has_sp(buf);
}

#endif
