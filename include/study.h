#ifndef STUDY_H_
#define STUDY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// include standard library
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/epoll.h>

typedef struct
{
    int epollfd;
    int ncap;
    struct epoll_event *events;
} stPoll_t;

typedef struct
{
    stPoll_t poll;
} stGlobal_t;

extern stGlobal_t StudyG;

enum stEvent_type
{
    ST_EVENT_NULL   = 0,
    ST_EVENT_DEAULT = 1u << 8,
    ST_EVENT_READ   = 1u << 9,
    ST_EVENT_WRITE  = 1u << 10,
    ST_EVENT_RDWR   = ST_EVENT_READ | ST_EVENT_WRITE,
    ST_EVENT_ERROR  = 1u << 11,
};

static inline uint64_t touint64(int fd, int id)
{
    uint64_t ret = 0;
    ret |= ((uint64_t)fd) << 32;
    ret |= ((uint64_t)id);

    return ret;
}

#endif /* STUDY_H_ */