#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "goat.h"
#include "core.h"

int core_thread_notify_fd;
pthread_t core_thread;

int goat_initialise(void) {
    int fd[2];

    if (0 != pipe(fd)) return -1;

    core_thread_notify_fd = fd[1];
    fcntl(core_thread_notify_fd, F_SETNOSIGPIPE, 1);

    pthread_mutex_lock(&core_state.mutex);
    core_state.running = 1;
    pthread_create(&core_thread, NULL, &_goat_core, &fd[0]);
    pthread_mutex_unlock(&core_state.mutex);

    return 0;
}

int goat_shutdown(void) {
    const char s[] = "shutdown\n"; // actual value is of no consequence

    write(core_thread_notify_fd, s, sizeof s);
    while (0 != pthread_mutex_trylock(&core_state.mutex)) {
        static const struct timespec delay = { 0, 200 * 1000 * 1000 };  // 0.2s
        nanosleep(&delay, NULL);
        write(core_thread_notify_fd, s, sizeof s);
    }
    core_state.running = 0;
    pthread_mutex_unlock(&core_state.mutex);
    pthread_join(core_thread, NULL);

    return 0;
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
    // validate:
    //   prefix/command cannot contain spaces
    //   if a param contains spaces it must be the last param
    //   sum of lengths must be <= 510... long line handling?
    return -1;
}

int goat_install_callback(goat_event event, goat_callback callback) {
    return -1;
}

int goat_uninstall_callback(goat_event event, goat_callback callback) {
    return -1;
}
