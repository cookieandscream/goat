#ifndef GOAT_CONTEXT_H
#define GOAT_CONTEXT_H

#include <pthread.h>
#include <stdlib.h>

#include <tls.h>

#include "goat.h"

#include "connection.h"

struct goat_context {
    pthread_rwlock_t    m_rwlock;
    Connection          **m_connections;
    size_t              m_connections_size;
    size_t              m_connections_count;
    GoatCallback        *m_callbacks;
    struct tls_config   *m_tls_config;
};

Connection *context_get_connection(GoatContext *context, int index);

#endif
