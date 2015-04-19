#include "goat.h"

#include "error.h"

const char *const error_strings[GOAT_E_LAST] = {
    [GOAT_E_STATE   - GOAT_E_FIRST] = "invalid connection state",
    [GOAT_E_UNREC   - GOAT_E_FIRST] = "unrecognised command string",
    [GOAT_E_INVMSG  - GOAT_E_FIRST] = "message is malformed",
};
