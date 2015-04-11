#ifndef GOAT_CONTEXT_H
#define GOAT_CONTEXT_H

#include <pthread.h>
#include <stdlib.h>

#include <tls.h>

#include "goat.h"

#include "connection.h"

struct s_goat_context {
    pthread_rwlock_t    m_rwlock;
    goat_connection_t   **m_connections;
    size_t              m_connections_size;
    size_t              m_connections_count;
    goat_callback_t     *m_callbacks;
    goat_error_t        m_error;
    struct tls_config   *m_tls_config;
};

goat_connection_t *context_get_connection(goat_context_t *context, int index);

#endif
