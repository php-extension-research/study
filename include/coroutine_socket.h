#ifndef COROUTINE_SOCKET_H
#define COROUTINE_SOCKET_H

#include "study.h"

namespace study { namespace coroutine {
class Socket
{
private:
    int sockfd;
public:
    Socket(int domain, int type, int protocol);
    Socket(int fd);
    ~Socket();
    int bind(int type, char *host, int port);
    int listen();
    int accept();
    ssize_t recv(void *buf, size_t len);
    ssize_t send(const void *buf, size_t len);
    int close();

    bool wait_event(int event);
};
}
}

#endif	/* COROUTINE_SOCKET_H */