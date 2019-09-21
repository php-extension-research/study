#include "coroutine_socket.h"
#include "coroutine.h"
#include "socket.h"
#include "log.h"

using study::Coroutine;
using study::coroutine::Socket;

char * Socket::read_buffer = nullptr;
size_t Socket::read_buffer_len = 0;
char * Socket::write_buffer = nullptr;
size_t Socket::write_buffer_len = 0;

Socket::Socket(int domain, int type, int protocol)
{
    sockfd = stSocket_create(domain, type, protocol);
    if (sockfd < 0)
    {
        return;
    }
    stSocket_set_nonblock(sockfd);
}

Socket::Socket(int fd)
{
    sockfd = fd;
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

    do
    {
        connfd = stSocket_accept(sockfd);
    } while (connfd < 0 && errno == EAGAIN && wait_event(ST_EVENT_READ));
    
    return connfd;
}

ssize_t Socket::recv(void *buf, size_t len)
{
    int ret;

    do
    {
        ret = stSocket_recv(sockfd, buf, len, 0);
    } while (ret < 0 && errno == EAGAIN && wait_event(ST_EVENT_READ));
   
    return ret;
}

ssize_t Socket::send(const void *buf, size_t len)
{
    int ret;

    do
    {
        ret = stSocket_send(sockfd, buf, len, 0);
    } while (ret < 0 && errno == EAGAIN && wait_event(ST_EVENT_WRITE));
    
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
    (StudyG.poll->event_num)++;

    co->yield();

    if (epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_DEL, sockfd, NULL) < 0)
    {
        stError("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return false;
    }
    return true;
}

int Socket::init_read_buffer()
{
    if (!read_buffer)
    {
        try
        {
            read_buffer = new char[65536];
        }
        catch(const std::bad_alloc& e)
        {
            stError("%s", e.what());
        }
        
        read_buffer_len = 65536;
    }

    return 0;
}

int Socket::init_write_buffer()
{
    if (!write_buffer)
    {
        try
        {
            write_buffer = new char[65536];
        }
        catch(const std::bad_alloc& e)
        {
            stError("%s", e.what());
        }
        
        write_buffer_len = 65536;
    }

    return 0;
}