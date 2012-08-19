
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
        FD_SET(notify_fd, &readfds);

        FD_ZERO(&writefds);

        // TODO loop over all the current connections and add their fds to the read/write sets

        // n.b. nfds = max(fds) + 1
        nfds = notify_fd + 1;

        if (select(nfds, &readfds, &writefds, NULL, NULL) > 0) {
            // there are descriptors ready for action, go and do stuff with them
        }

        pthread_mutex_lock(&core_state.mutex);
    }
    pthread_mutex_unlock(&core_state.mutex);

    return NULL;
}
