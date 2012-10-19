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
static int _event_parse_msg(const char *msg, char **prefix, char **command, char ***params);
static goat_event_t _event_get_type(const char *prefix, const char *command, const char **params);

int event_process(goat_context_t *context, int connection, const char *message) {
    assert(context != NULL);
    assert(message != NULL);

    char *prefix, *command, **params;

    int ret = _event_parse_msg(message, &prefix, &command, &params);
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

int _event_parse_msg(const char *msg, char **r_prefix, char **r_command, char ***r_params) {
    char *prefix = NULL, *command = NULL, **params = NULL;
    char *msg_copy, *remainder, *token, *end;
    int ret = -1;

    remainder = msg_copy = strdup(msg);
    if (remainder == NULL)  return GOAT_E_NOMEM;

    // chomp crlf
    end = strchr(remainder, '\0');
    if (*(end - 1) == '\x0a')  *(--end) = '\0';
    if (*(end - 1) == '\x0d')  *(--end) = '\0';

    // [ ':' prefix SPACE ]
    if (*remainder == ':') {
        ++ remainder;
        token = strsep(&remainder, " ");
        if (*token == '\0') {
            ret = GOAT_E_INVMSG;
            goto cleanup;
        }
        prefix = strdup(token);
        if (prefix == NULL) {
            ret = GOAT_E_NOMEM;
            goto cleanup;
        }
    }

    // command
    if ((token = strsep(&remainder, " "))) {
        if (*token == '\0') {
            ret = GOAT_E_INVMSG;
            goto cleanup;
        }
        command = strdup(token);
        if (command == NULL) {
            ret = GOAT_E_NOMEM;
            goto cleanup;
        }
    }
    else {
        ret = GOAT_E_INVMSG;
        goto cleanup;
    }

    // *14( SPACE middle ) [ SPACE ":" trailing ]
    // 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
    if (remainder) {
        params = calloc(16, sizeof(char *));
        if (params == NULL) {
            ret = GOAT_E_NOMEM;
            goto cleanup;
        }

        unsigned i = 0;
        while (i < 14 && remainder != NULL) {
            if (*remainder == ':')  break;
            token = strsep(&remainder, " ");
            params[i] = strdup(token);
            if (params[i] == NULL) {
                ret = GOAT_E_NOMEM;
                goto cleanup;
            }
            ++ i;
        }

        if (remainder) {
            if (*remainder == ':')  ++remainder;
            params[i] = strdup(remainder);
            if (params[i] == NULL) {
                ret = GOAT_E_NOMEM;
                goto cleanup;
            }
        }
    }

    free(msg_copy);
    *r_prefix = prefix;
    *r_command = command;
    *r_params = params;
    return 0;

cleanup:
    if (params) {
        char **p = params;
        while (*p)  free(*(p++));
        free(params);
    }
    if (command)  free(command);
    if (prefix)  free(prefix);

    free(msg_copy);
    return ret;
}

int _event_commands_cmp(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **)b);
}
