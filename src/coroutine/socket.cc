#include "coroutine_socket.h"
#include "coroutine.h"
#include "socket.h"

using study::Coroutine;
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

Socket::~Socket()
{
}

int Socket::bind(int type, char *host, int port)
{
    return stSocket_bind(sockfd, type, host, port);
}

int Socket::listen()
{
    return stSocket_listen(sockfd);
}

int Socket::accept()
{
    int connfd;

    connfd = stSocket_accept(sockfd);
    if (connfd < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_READ);
        connfd = stSocket_accept(sockfd);
    }

    return connfd;
}

ssize_t Socket::recv(void *buf, size_t len)
{
    int ret;

    ret = stSocket_recv(sockfd, buf, len, 0);
    if (ret < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_READ);
        ret = stSocket_recv(sockfd, buf, len, 0);
    }
    return ret;
}

ssize_t Socket::send(const void *buf, size_t len)
{
    int ret;

    ret = stSocket_send(sockfd, buf, len, 0);
    if (ret < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_WRITE);
        ret = stSocket_send(sockfd, buf, len, 0);
    }
    return ret;
}

int Socket::close()
{
    return stSocket_close(sockfd);
}

bool Socket::wait_event(int event)
{
    long id;
    Coroutine* co;
    epoll_event *ev;

    co = Coroutine::get_current();
    id = co->get_cid();

    if (!StudyG.poll)
    {
        init_stPoll();
    }

    ev = StudyG.poll->events;

    ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
    ev->data.u64 = touint64(sockfd, id);
    epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_ADD, sockfd, ev);

    co->yield();
    return true;
}