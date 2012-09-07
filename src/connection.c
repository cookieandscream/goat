#include <assert.h>

#include "connection.h"

typedef void (*state_function)(goat_connection *, int, int);

static void _state_disconnected(goat_connection *, int, int);
static void _state_resolving(goat_connection *, int, int);
static void _state_connecting(goat_connection *, int, int);
static void _state_connected(goat_connection *, int, int);
static void _state_disconnecting(goat_connection *, int, int);

const static state_function do_state[] = {
    _state_disconnected,
    _state_resolving,
    _state_connecting,
    _state_connected,
    _state_disconnecting,
};

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

    if (0 == pthread_mutex_lock(&conn->mutex)) {
        switch (conn->state) {
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
                do_state[conn->state](conn, socket_readable, socket_writeable);
                break;

            case GOAT_CONN_ERROR:
                break;
        }
        pthread_mutex_unlock(&conn->mutex);
    }

    return (conn->state == GOAT_CONN_ERROR) ? -1 : 0;
}

void _state_disconnected(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
}

void _state_resolving(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_RESOLVING);
    // see if we've got a result yet
    if (0) {
        // got a result!  start connecting
        conn->state = GOAT_CONN_CONNECTING;
    }
}

void _state_connecting(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTING);
    if (socket_writeable) {
        conn->state = GOAT_CONN_CONNECTED;
    }
}

void _state_connected(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);
    if (socket_readable) {
        // read data into the read buffer
        // if it's been disconnected, bump the state to disconnecting
    }
    if (socket_writeable) {
        // burn through write buffer
        // if it's been disconnected, bump the state to disconnecting
    }
}

void _state_disconnecting(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_DISCONNECTING);
    // any processing we need to do during disconnect
    conn->state = GOAT_CONN_DISCONNECTED;
}
