#ifndef GOAT_TRESOLVER_H
#define GOAT_TRESOLVER_H

#include <netdb.h>

typedef struct resolver_state ResolverState;

int resolver_getaddrinfo(ResolverState **statep, const char *hostname,
    const char *servname, struct addrinfo **resp);

int resolver_cancel(ResolverState **statep);

#endif
