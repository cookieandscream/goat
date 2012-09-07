
#include <sys/select.h>

#include <pthread.h>

#include "core.h"

goat_core_state core_state = { PTHREAD_MUTEX_INITIALIZER, 0 };

goat_core_connection_list core_connection_list = { PTHREAD_MUTEX_INITIALIZER, 0, NULL };

void *_goat_core (void *arg) {
    const int notify_fd = *(int *)arg;
    fd_set readfds, writefds;
    int nfds;
    const static struct timeval ms200 = { .tv_sec = 0, .tv_usec = 200 * 1000 * 1000 };
    struct timeval timeout, *timeout_ptr;

    pthread_mutex_lock(&core_state.mutex);
    while (core_state.running) {
        pthread_mutex_unlock(&core_state.mutex);

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(notify_fd, &readfds);
        nfds = notify_fd;

        timeout_ptr = NULL;

        pthread_mutex_lock(&core_connection_list.mutex);
        for (unsigned i = 0; i < core_connection_list.connection_pool_size; i++) {
            if (core_connection_list.connection_pool[i] != NULL) {
                goat_connection *conn = core_connection_list.connection_pool[i];

                if (conn_wants_read(conn)) {
                    FD_SET(conn->socket, &readfds);
                }
                if (conn_wants_write(conn)) {
                    FD_SET(conn->socket, &writefds);
                }
                if (!timeout_ptr && conn_wants_timeout(conn)) {
                    timeout = ms200;
                    timeout_ptr = &timeout;
                }

                nfds = (conn->socket > nfds ? conn->socket : nfds);
            }
        }
        pthread_mutex_unlock(&core_connection_list.mutex);

        if (select(nfds + 1, &readfds, &writefds, NULL, timeout_ptr) >= 0) {
            // n.b. we pump the connection even if select timed out
            for (unsigned i = 0; i < core_connection_list.connection_pool_size; i++) {
                if (core_connection_list.connection_pool[i] != NULL) {
                    goat_connection *conn = core_connection_list.connection_pool[i];

                    int read_ready = FD_ISSET(conn->socket, &readfds);
                    int write_ready = FD_ISSET(conn->socket, &writefds);
                    conn_pump_socket(conn, read_ready, write_ready);
                }
            }
        }

        pthread_mutex_lock(&core_state.mutex);
    }
    pthread_mutex_unlock(&core_state.mutex);

    return NULL;
}
