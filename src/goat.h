#ifndef GOAT_H
#define GOAT_H

#include <sys/time.h> /* struct timeval */

typedef struct s_goat_context goat_context_t;

typedef void (*goat_callback_t)(
    goat_context_t          *context,
    int                     connection,
    const char *restrict    prefix,
    const char *restrict    command,
    const char **restrict   params
);

typedef enum {
    GOAT_EVENT_GENERIC = 0,

    GOAT_EVENT_LAST /* don't use; keep last */
} goat_event_t;

typedef enum {
    GOAT_E_NONE = 0,        // everything is fine
    GOAT_E_ERRORINV = 1,    // invalid argument passed to goat_error
    GOAT_E_STATE = 2,       // invalid connection state
    GOAT_E_NOMEM = 3,       // couldn't allocate memory
    GOAT_E_INVMSG = 4,      // message is malformed

    GOAT_E_LAST /* don't use; keep last */
} goat_error_t;

goat_context_t *goat_context_new();
int goat_context_delete(goat_context_t *context);

goat_error_t goat_error(goat_context_t *context, int connection);
const char *goat_strerror(goat_error_t error);
int goat_reset_error(goat_context_t *context, int connection);

int goat_connection_new(goat_context_t *context);
int goat_connection_delete(goat_context_t *context, int connection);

int goat_connect(goat_context_t *context, int connection, const char *hostname, int port, int ssl);
int goat_disconnect(goat_context_t *connect, int connection);
int goat_is_connected(goat_context_t *connect, int connection);
int goat_get_hostname(goat_context_t *connect, int connection, char **hostname);

int goat_send_message(
    goat_context_t  * context,
    int             connection,
    const char      *restrict prefix,
    const char      *restrict command,
    const char      **restrict params
);

int goat_install_callback(goat_context_t *context, goat_event_t event, goat_callback_t callback);
int goat_uninstall_callback(goat_context_t *context, goat_event_t event, goat_callback_t callback);

int goat_select_fds(goat_context_t *context, fd_set *restrict readfds, fd_set *restrict writefds);
int goat_tick(goat_context_t *context, struct timeval *timeout);
int goat_dispatch_events(goat_context_t *context);

#endif
