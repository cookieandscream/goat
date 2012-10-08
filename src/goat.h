#ifndef GOAT_H
#define GOAT_H

typedef struct s_goat_context goat_context;

typedef void (*goat_callback)(
    goat_context *,
    int,
    const char *restrict,
    const char *restrict,
    const char **restrict
);

typedef enum {
    GOAT_EVENT_GENERIC,
} goat_event;

goat_context *goat_context_new();
int goat_context_delete(goat_context *);

const char *goat_error(goat_context *);

int *goat_connection_new(goat_context *);
int goat_connection_delete(goat_context *, int);

int goat_connect(goat_context *restrict, int, const char *restrict, int, int);
int goat_disconnect(goat_context *restrict, int);
int goat_is_connected(goat_context *restrict, int);
int goat_get_hostname(goat_context *restrict, int, char **restrict);

int goat_send_message(
    goat_context *restrict,
    int,
    const char *restrict,
    const char *restrict,
    const char **restrict
);

int goat_install_callback(goat_context *restrict, goat_event, goat_callback);
int goat_uninstall_callback(goat_context *restrict, goat_event, goat_callback);

int goat_tick(goat_context *restrict, struct timeval *restrict);
int goat_dispatch_events(goat_context *restrict);

#endif
