
#include "goat.h"
#include "core.h"

int goat_inititialise(void) {
    return -1;
}

int goat_shutdown(void) {
    return -1;
}

goat_handle goat_alloc(void) {
    return -1;
}

int goat_destroy(goat_handle handle) {
    return -1;
}

int goat_connect(goat_handle handle, const char *hostname, int port) {
    return -1;
}

int goat_disconnect(goat_handle handle) {
    return -1;
}

int goat_is_connected(goat_handle handle) {
    return -1;
}

int goat_get_connected_hostname(goat_handle handle, char **retval) {
    return -1;
}

int goat_send_message(
        goat_handle handle,
        const char *restrict prefix,
        const char *restrict command,
        const char **restrict params
) {
    return -1;
}

int goat_install_callback(goat_event event, goat_callback callback) {
    return -1;
}

int goat_uninstall_callback(goat_event event, goat_callback callback) {
    return -1;
}
