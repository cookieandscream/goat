#include "goat.h"

#include "error.h"

const char *const error_strings[] = {
    [GOAT_E_NONE]       = NULL,
    [GOAT_E_ERRORINV]   = "goat_error was invoked with an invalid argument",
    [GOAT_E_STATE]      = "invalid connection state",

    [GOAT_E_LAST]       = NULL,
};
