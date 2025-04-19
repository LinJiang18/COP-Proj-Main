#ifndef ENDPOINT_H
#define ENDPOINT_H
#include <netinet/in.h>
typedef struct sockaddr_in endpoint_t;

int ep_equal(endpoint_t lp, endpoint_t rp);

/* string is host:port format */
/* IPV4 ONLY */
char *ep_tostring(endpoint_t ep);
endpoint_t ep_fromstring(const char *tuple);
endpoint_t ep_frompair(const char *ip, short port);

#endif
