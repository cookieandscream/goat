#include <assert.h>

#include "context.h"

goat_connection_t *context_get_connection(goat_context_t *context, int index) {
    assert(NULL != context);

    goat_connection_t *conn = NULL;

    if (0 == pthread_rwlock_rdlock(&context->m_rwlock)) {
        if (index >= 0
            && index < (int) context->m_connections_size
            && context->m_connections[index] != NULL
        ) {
            conn = context->m_connections[index];
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }

    return conn;
}
