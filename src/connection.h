#ifndef GOAT_CONNECTION_H
#define GOAT_CONNECTION_H

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <pthread.h>

#include "goat.h"

typedef enum {
    GOAT_CONN_DISCONNECTED  = 0,
    GOAT_CONN_RESOLVING,
    GOAT_CONN_CONNECTING,
    GOAT_CONN_CONNECTED,
    GOAT_CONN_DISCONNECTING,

    // keep error as last
    GOAT_CONN_ERROR
} goat_conn_state_t;

typedef struct s_str_queue_entry {
    STAILQ_ENTRY(s_str_queue_entry) entries;
    size_t len;
    char str[0];
} str_queue_entry_t;

typedef STAILQ_HEAD(s_str_queue_head, s_str_queue_entry) str_queue_head_t;

typedef struct {
    int handle;
    pthread_mutex_t mutex;
    int socket;
    struct sockaddr *address;
    socklen_t address_len;
    goat_conn_state_t state;
    void *state_data;
    int ssl;
    str_queue_head_t write_queue;
    str_queue_head_t read_queue;
} goat_connection_t;

int conn_init(goat_connection_t *connection, int handle);
int conn_destroy(goat_connection_t *connection);

int conn_connect(goat_connection_t *); // FIXME
int conn_disconnect(goat_connection_t *); // FIXME

int conn_wants_read(const goat_connection_t *);
int conn_wants_write(const goat_connection_t *);
int conn_wants_timeout(const goat_connection_t *);

int conn_queue_message(goat_connection_t *restrict, const char *restrict, const char *restrict, const char **restrict);

int conn_tick(goat_connection_t *conn, int socket_readable, int socket_writeable);

#endif
