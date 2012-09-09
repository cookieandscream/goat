#ifndef GOAT_H
#define GOAT_H

typedef int goat_handle;
typedef void (*goat_callback)(goat_handle, const char *restrict, const char *restrict, const char **restrict);
typedef enum {
    // TODO define some events
    _dummy,
} goat_event;

int goat_initialise(void);
int goat_shutdown(void);

goat_handle goat_alloc(void);
int goat_destroy(goat_handle);

int goat_connect(goat_handle, const char *, int, int);
int goat_disconnect(goat_handle);
int goat_is_connected(goat_handle);
int goat_get_connected_hostname(goat_handle, char **);

int goat_send_message(goat_handle, const char *restrict, const char *restrict, const char **restrict);

int goat_install_callback(goat_event, goat_callback);
int goat_uninstall_callback(goat_event, goat_callback);

#endif
