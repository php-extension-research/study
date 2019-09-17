#include "study.h"
#include "uv.h"
#include "coroutine.h"
#include "log.h"

using study::Coroutine;

stGlobal_t StudyG;

int init_stPoll()
{
    size_t size;

    StudyG.poll = (stPoll_t *)malloc(sizeof(stPoll_t));

    if (StudyG.poll == NULL)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return -1;
    }

    StudyG.poll->epollfd = epoll_create(256);
    if (StudyG.poll->epollfd  < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        free(StudyG.poll);
        return -1;
    }

    StudyG.poll->ncap = 16;
    size = sizeof(struct epoll_event) * StudyG.poll->ncap;
    StudyG.poll->events = (struct epoll_event *) malloc(size);
    memset(StudyG.poll->events, 0, size);

    return 0;
}

int free_stPoll()
{
    free(StudyG.poll->events);
    free(StudyG.poll);
    return 0;
}

int st_event_init()
{
    if (!StudyG.poll)
    {
        init_stPoll();
    }

    return 0;
}

typedef enum
{
    UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
    UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

extern "C" void uv__run_timers(uv_loop_t* loop);
extern "C" uint64_t uv__hrtime(uv_clocktype_t type);
extern "C" int uv__next_timeout(const uv_loop_t* loop);

int st_event_wait()
{
    uv_loop_t *loop = uv_default_loop();

    if (!StudyG.poll)
    {
        stError("Need to call st_event_init first.");
    }

    while (loop->stop_flag == 0)
    {
        int n;
        int timeout;
        epoll_event *events;

        timeout = uv__next_timeout(loop);
        events = StudyG.poll->events;
        n = epoll_wait(StudyG.poll->epollfd, events, StudyG.poll->ncap, timeout);

        for (int i = 0; i < n; i++) {
            int fd;
            int id;
            struct epoll_event *p = &events[i];
            uint64_t u64 = p->data.u64;
            Coroutine *co;

            fromuint64(u64, &fd, &id);
            co = Coroutine::get_by_cid(id);
            co->resume();
        }

        loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
        uv__run_timers(loop);

        if (uv__next_timeout(loop) < 0 && !StudyG.poll)
        {
            uv_stop(loop);
        }
    }

    st_event_free();

    return 0;
}

int st_event_free()
{
    free_stPoll();
    return 0;
} 