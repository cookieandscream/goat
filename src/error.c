#include "goat.h"

#include "error.h"

const char *const error_strings[GOAT_E_LAST] = {
    [GOAT_E_STATE   - GOAT_E_FIRST] = "invalid connection state",
    [GOAT_E_UNREC   - GOAT_E_FIRST] = "unrecognised command string",
    [GOAT_E_MSGLEN  - GOAT_E_FIRST] = "message is or would be too long",
    [GOAT_E_NOTAG   - GOAT_E_FIRST] = "tag does not exist",
    [GOAT_E_NOTAGVAL- GOAT_E_FIRST] = "tag does not have a value",

};
