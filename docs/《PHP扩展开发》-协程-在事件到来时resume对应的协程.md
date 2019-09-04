# 在事件到来时resume对应的协程

我们在前面已经实现了`accept`的协程化，但是我们还没有实现客户端连接到来时`resume`对应的协程。我们这篇文章来实现一下。

在文件`study_coroutine.cc`里面：

```cpp
int PHPCoroutine::scheduler()
{
    uv_loop_t *loop = uv_default_loop();

    if (!StudyG.poll)
    {
        init_stPoll();
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

    free_stPoll();

    return 0;
}
```

其中，

```cpp
if (!StudyG.poll)
{
  	init_stPoll();
}
```

检测我们的`StudyG.poll`是否初始化，如果没有，那么进行初始化。

```cpp
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
```

这一段实际上就是我们`resume`对应协程的核心代码了。当事件触发的时候，我们可以获取到我们在`wait_event`里面设置的这个`p->data.u64`数据，我们可以通过这个数据获取到对应的那个协程`id`。我们在之前的文章里面讲过这个思路，这里不重复啰嗦。

这里，我们把初始化`StudyG.poll`的操作封装了起来，因为我们可能会在其他的地方使用到初始化。我们在文件`include/study.h`里面实现：

```cpp
static inline void init_stPoll()
{
    size_t size;

    StudyG.poll = (stPoll_t *)malloc(sizeof(stPoll_t));

    StudyG.poll->epollfd = epoll_create(256);
    StudyG.poll->ncap = 16;
    size = sizeof(struct epoll_event) * StudyG.poll->ncap;
    StudyG.poll->events = (struct epoll_event *) malloc(size);
    memset(StudyG.poll->events, 0, size);
}
```

这里，我们把`StudyG.poll`定义为了指针的类型，因为这样好判断它是否进行过初始化的操作。所以，我们需要修改它的定义，在文件`include/study.h`里面：

```cpp
typedef struct
{
    stPoll_t *poll;
} stGlobal_t;
```

我们改成了指针的类型。

然后，我们需要修改`Socket::wait_event`里面的代码，把`StudyG.poll`改成指针的用法：

```cpp
bool Socket::wait_event(int event)
{
    long id;
    Coroutine* co;
    epoll_event *ev;

    co = Coroutine::get_current();
    id = co->get_cid();

    ev = StudyG.poll->events; // 修改地方

    ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
    ev->data.u64 = touint64(sockfd, id);
    epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_ADD, sockfd, ev); // 修改地方

    co->yield();
    return true;
}
```

接着，我们去实现`fromuint64`，在文件`include/study.h`里面：

```cpp
static inline void fromuint64(uint64_t v, int *fd, int *id)
{
    *fd = (int)(v >> 32);
    *id = (int)(v & 0xffffffff);
}
```

然后，我们需要去实现：

```cpp
static study::Coroutine *study::Coroutine::get_by_cid(long cid)
```

它可以通过协程的`id`，来获取对应的协程对象。我们在文件`include/coroutine.h`的类`study::Coroutine`里面实现：

```cpp
public:
    static inline Coroutine* get_by_cid(long cid)
    {
        auto i = coroutines.find(cid);
        return i != coroutines.end() ? i->second : nullptr;
    }
```

就是直接去`coroutines`里面查找即可。

最后，我们在调度器结束的时候，需要释放掉我们的`StudyG.poll`，我们在文件`include/study.h`里面实现`free_stPoll`：

```cpp
static inline void free_stPoll()
{
    free(StudyG.poll->events);
    free(StudyG.poll);
}
```

`OK`，到这里，我们算是实现完了我们的调度器。

我们来重新编译、安装扩展：

```cpp
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

编译成功，符合预期。

