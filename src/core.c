
#include <sys/select.h>

#include <pthread>

#include "core.h"

goat_core_state core_state = { PTHREAD_MUTEX_INITIALIZER, 0 };

goat_core_connection_list core_connection_list = { PTHREAD_MUTEX_INITIALIZER, 0, NULL };

void *_goat_core (void *arg) {
    const int notify_fd = *(int *)arg;
    fd_set readfds, writefds;
    int nfds;

    pthread_mutex_lock(&core_state.mutex);
    while (core_state.running) {
        pthread_mutex_unlock(&core_state.mutex);

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(notify_fd, &readfds);
        nfds = notify_fd;

        pthread_mutex_lock(&core_connection_list.mutex);
        for (unsigned i = 0; i < core_connection_list.connection_pool_size; i++) {
            if (core_connection_list.connection_pool[i] != NULL) {
                FD_SET(core_connection_list.connection_pool[i].socket, &readfds);
                FD_SET(core_connection_list.connection_pool[i].socket, &writefds);
                if (core_connection_list.connection_pool[i].socket > nfds) {
                    nfds = core_connection_list.connection_pool[i].socket;
                }
            }
        }
        pthread_mutex_unlock(&core_connection_list.mutex);

        if (select(nfds + 1, &readfds, NULL, NULL, NULL) > 0) {
            // there are descriptors ready to read, go and process them
        }

        // TODO if we have pending outgoing messages, try to send them

        pthread_mutex_lock(&core_state.mutex);
    }
    pthread_mutex_unlock(&core_state.mutex);

    return NULL;
}
