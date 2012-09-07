#ifndef GOAT_CONNECTION_H
#define GOAT_CONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>

#include <pthread.h>

#include "goat.h"

typedef enum {
    GOAT_CONN_ERROR         = -1,
    GOAT_CONN_DISCONNECTED  = 0,
    GOAT_CONN_RESOLVING,
    GOAT_CONN_CONNECTING,
    GOAT_CONN_CONNECTED,
    GOAT_CONN_DISCONNECTING,
} goat_connection_state;

typedef struct {
    const goat_handle handle;
    pthread_mutex_t mutex;
    int socket;
    struct sockaddr *address;
    socklen_t address_len;
    goat_connection_state state;
    int ssl;
    int write_queue;
    int read_queue;
} goat_connection;

int conn_init(goat_connection *);
int conn_destroy(goat_connection *);

int conn_connect(goat_connection *); // FIXME
int conn_disconnect(goat_connection *); // FIXME

int conn_wants_read(const goat_connection *);
int conn_wants_write(const goat_connection *);
int conn_wants_timeout(const goat_connection *);

int conn_queue_message(goat_connection *restrict, const char *restrict, const char *restrict, const char **restrict);

int conn_pump_socket(goat_connection *, int, int); // read ready, write ready

#endif
