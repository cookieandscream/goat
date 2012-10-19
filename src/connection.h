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
    size_t  len;
    int     has_eol;
    char    str[0];
} str_queue_entry_t;

typedef STAILQ_HEAD(s_str_queue_head, s_str_queue_entry) str_queue_head_t;

typedef struct {
    int                 m_handle;
    pthread_mutex_t     m_mutex;
    int                 m_socket;
    struct sockaddr     *m_address;
    socklen_t           m_address_len;
    goat_conn_state_t   m_state;
    void                *m_state_data;
    int                 m_ssl;
    str_queue_head_t    m_write_queue;
    str_queue_head_t    m_read_queue;
    int                 m_socket_is_readable;
    int                 m_socket_is_writeable;
    goat_error_t        m_error;
} goat_connection_t;

int conn_init(goat_connection_t *conn, int handle);
int conn_destroy(goat_connection_t *conn);

int conn_connect(goat_connection_t *); // FIXME
int conn_disconnect(goat_connection_t *); // FIXME

int conn_wants_read(const goat_connection_t *);
int conn_wants_write(const goat_connection_t *);
int conn_wants_timeout(const goat_connection_t *);

int conn_reset_error(goat_connection_t *conn);

int conn_queue_message(goat_connection_t *restrict, const char *restrict, const char *restrict, const char **restrict);

char *conn_pop_message(goat_connection_t *conn);

int conn_tick(goat_connection_t *conn, int socket_readable, int socket_writeable);

#endif
