#ifndef GOAT_UTIL_H
#define GOAT_UTIL_H

#include <string.h>

#define str_has_crlf(s)     (strcspn((s), "\x0d\x0a") < strlen((s)))
#define str_has_sp(s)       (NULL != strchr((s), ' '))

int strn_has_crlf(const char *str, size_t len);
int strn_has_sp(const char *str, size_t len);

#endif
