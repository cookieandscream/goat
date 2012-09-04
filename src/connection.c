#include <assert.h>

#include "connection.h"

int conn_init(goat_connection *conn) {
    assert(conn != NULL);
    return -1; // FIXME
}

int conn_destroy(goat_connection *conn) {
    assert(conn != NULL);
    return -1; // FIXME
}

int conn_wants_read(const goat_connection *conn) {
    assert(conn != NULL);

    return conn->state >= GOAT_CONN_CONNECTING;
}

int conn_wants_write(const goat_connection *conn) {
    assert(conn != NULL);
    if (conn->state == GOAT_CONN_CONNECTING) {
        return 1;
    }
    else if (conn->state == GOAT_CONN_CONNECTED && conn->write_queue) { // FIXME
        return 1;
    }
    else {
        return 0;
    }
}

int conn_pump_socket(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);
    int result = -1;

    if (0 == pthread_mutex_lock(&conn->mutex)) {
        switch (conn->state) {
            case GOAT_CONN_CONNECTING:
                if (socket_writeable) {
                    conn->state = GOAT_CONN_CONNECTED;
                }
                result = 0;
                break;
            case GOAT_CONN_CONNECTED:
                if (socket_readable) {
                    // read data into the read buffer
                }
                if (socket_writeable) {
                    // burn through write buffer
                }
                result = 0;
                break;
            case GOAT_CONN_DISCONNECTING:
            case GOAT_CONN_ERROR:
                result = -1; // FIXME
        }
        pthread_mutex_unlock(&conn->mutex);
    }

    return result;
}
