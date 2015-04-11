#include "util.h"

int strn_has_crlf(const char *str, size_t len) {
    char buf[len + 1];

    strncpy(buf, str, len);
    buf[len] = '\0';

    return str_has_crlf(buf);
}

int strn_has_sp(const char *str, size_t len) {
    char buf[len + 1];

    strncpy(buf, str, len);
    buf[len] = '\0';

    return str_has_sp(buf);
}
