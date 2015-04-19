#include "goat.h"

#include "error.h"

const char *const error_strings[GOAT_E_LAST] = {
    [GOAT_E_NONE]       = NULL,
    [GOAT_E_INVCONTEXT] = "invalid context argument",
    [GOAT_E_INVCONN]    = "invalid connection argument",
    [GOAT_E_STATE]      = "invalid connection state",
    [GOAT_E_NOMEM]      = "out of memory",
    [GOAT_E_INVMSG]     = "message is malformed",
};
