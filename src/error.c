#include "goat.h"

#include "error.h"

const char *const error_strings[GOAT_E_LAST] = {
    [GOAT_E_INVCONTEXT  - GOAT_E_FIRST] = "invalid context argument",
    [GOAT_E_INVCONN     - GOAT_E_FIRST] = "invalid connection argument",
    [GOAT_E_STATE       - GOAT_E_FIRST] = "invalid connection state",
    [GOAT_E_NOMEM       - GOAT_E_FIRST] = "out of memory",
    [GOAT_E_INVMSG      - GOAT_E_FIRST] = "message is malformed",
};
