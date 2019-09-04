#ifndef SOCKET_H
#define SOCKET_H

#include "study.h"

enum stSocket_type
{
    ST_SOCK_TCP          =  1,
    ST_SOCK_UDP          =  2,
};

int stSocket_create(int domain, int type, int protocol);
int stSocket_bind(int sock, int type, char *host, int port);
int stSocket_listen(int sock);
int stSocket_accept(int sock);

ssize_t stSocket_recv(int sock, void *buf, size_t len, int flag);
ssize_t stSocket_send(int sock, const void *buf, size_t len, int flag);
int stSocket_set_nonblock(int sock);

#endif	/* SOCKET_H */
