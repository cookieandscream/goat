#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"

static const goat_callback_t event_default_callbacks[GOAT_EVENT_LAST] = {

};

static const char *const event_commands[] = {
    // keep this sorted
    "MODE",
    "PRIVMSG",
};

static int _event_commands_cmp(const void *a, const void *b);
static int _event_parse_message(const char *message, char **prefix, char **command, char ***params);
static goat_event_t _event_get_type(const char *prefix, const char *command, const char **params);

int event_process(goat_context_t *context, int connection, const char *message) {
    assert(context != NULL);
    assert(message != NULL);

    char *prefix, *command, **params;

    int ret = _event_parse_message(message, &prefix, &command, &params);
    if (ret != 0)  return ret;

    goat_event_t event = _event_get_type(prefix, command, params);  // FIXME warning?

    // FIXME if there's a callback for it, call that

    // FIXME or if there's a default callback for it, call that

    // FIXME or if there's a generic callback, call that

    // FIXME or if there's a default generic callback, call that

    return -1; // FIXME
}

goat_event_t _event_get_type(const char *prefix, const char *command, const char **params) {
    return GOAT_EVENT_GENERIC; // FIXME
}

int _event_parse_message(const char *message, char **r_prefix, char **r_command, char ***r_params) {
    char *prefix = NULL, *command = NULL, **params = NULL;
    const char *p1, *p2, *eol;
    size_t len;
    int ret = -1;

    eol = strchr(message, '\x0d');
    if (eol == NULL)  eol = strchr(message, '\x0a');
    if (eol == NULL)  eol = strchr(message, '\0');

    p1 = message;

    // [ ':' prefix SPACE ]
    if (*p1 == ':') {
        ++ p1;
        p2 = strchr(p1, ' ');
        if (p2 == NULL) { ret = GOAT_E_INVMSG; goto cleanup; }
        len = p2 - p1;

        prefix = malloc(len + 1);
        if (prefix == NULL) { ret = GOAT_E_NOMEM; goto cleanup; }

        strncpy(prefix, p1, len);
        prefix[len] = '\0';

        p1 = p2 + 1;
        if (p1 >= eol) { ret = GOAT_E_INVMSG; goto cleanup; }
    }

    // command
    p2 = strchr(p1, ' ');
    if (p2 == NULL)  p2 = eol;
    len = p2 - p1;

    command = malloc(len + 1);
    if (command == NULL) { ret = GOAT_E_NOMEM; goto cleanup; }

    strncpy(command, p1, len);
    command[len] = '\0';

    if (p2 == eol)  goto finished;
    p1 = p2;

    // *14( SPACE middle ) [ SPACE ":" trailing ]
    // 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
    params = calloc(16, sizeof(char *));
    if (params == NULL) { ret = GOAT_E_NOMEM; goto cleanup; }
    unsigned i = 0;
    while (i < 14 && *p1 == ' ') {
        ++ p1;
        if (*p1 == ':')  goto trailing;

        p2 = strchr(p1, ' ');
        if (p2 == NULL)  p2 = eol;
        len = p2 - p1;

        params[i] = malloc(len + 1);
        if (params[i] == NULL) { ret = GOAT_E_NOMEM; goto cleanup; }

        strncpy(params[i], p1, len);
        params[i][len] = '\0';

        if (p2 == eol)  goto finished;
        p1 = p2;
        ++ i;
    }

trailing:
    ++ p1;
    p2 = eol;
    len = p2 - p1;

    params[i] = malloc(len + 1);
    if (params[i] == NULL) { ret = GOAT_E_NOMEM; goto cleanup; }

    strncpy(params[i], p1, len);
    params[i][len] = '\0';

finished:
    *r_prefix = prefix;
    *r_command = command;
    *r_params = params;
    return 0;

cleanup:
    if (prefix)  free(prefix);
    if (command)  free(command);
    if (params) {
        char **p = params;
        while (*p) {
            free(*p);
            ++p;
        }
        free(params);
    }
    return ret;
}

int _event_commands_cmp(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **)b);
}
