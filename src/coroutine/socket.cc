#include "coroutine_socket.h"
#include "socket.h"

using study::coroutine::Socket;

Socket::Socket(int domain, int type, int protocol)
{
    sockfd = stSocket_create(domain, type, protocol);
    if (sockfd < 0)
    {
        return;
    }
    stSocket_set_nonblock(sockfd);
}

int Socket::bind(int type, char *host, int port)
{
    return stSocket_bind(sockfd, type, host, port);
}

int Socket::listen()
{
    return stSocket_listen(sockfd);
}