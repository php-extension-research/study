# 修复一些bug（八）

这篇文章，我们来修复一个协程`sleep`的问题，有如下测试脚本：

```php
<?php

study_event_init();

Sgo(function () {
    var_dump(1);
    Sco::sleep(1);
    var_dump(2);
});

Sgo(function () {
    var_dump(3);
    Sco::sleep(1);
    var_dump(4);
});

study_event_wait();
```

运行脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(3)
int(2)
int(4)

```

我们发现，这个程序会一直卡住，不会结束。问题出在了`study_event_wait`里面，我们来看看源码：

```cpp
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
```

当调用完：

```cpp
uv__run_timers(loop);
```

我们的两个协程跑完之后，就会执行语句

```cpp
if (uv__next_timeout(loop) < 0 && !StudyG.poll)
{
    uv_stop(loop);
}
```

因为我们的定时器已经跑完了，所以`uv__next_timeout(loop)`肯定是返回-1，但是，因为`StudyG.poll`没有被释放掉，所以`!StudyG.poll`肯定是`false`，所以，就不会去执行`uv_stop(loop);`，所以`loop->stop_flag`还是`0`。所以，又再次进入的`while`循环，然后就执行代码：

```cpp
n = epoll_wait(StudyG.poll->epollfd, events, StudyG.poll->ncap, timeout);
```

因为此时没有定时器了，所以这里的`timeout`是`-1`。因此，`epoll_wait`会阻塞住，除非有其他事件（例如网络事件）的触发，这个函数才会返回。但是，我们的这个脚本没涉及到网络部分，所以这个程序就会一直的阻塞住，不会结束。因此，我们需要修改代码：

```cpp
if (uv__next_timeout(loop) < 0 && !StudyG.poll)
{
    uv_stop(loop);
}
```

我们增加一个事件计数的功能。在文件`include/study.h`里面：

```cpp
typedef struct
{
    int epollfd;
    int ncap;
    int event_num; // 新增加的一行
    struct epoll_event *events;
} stPoll_t;
```

然后再增加一个标识循环结束的标识：

```cpp
typedef struct
{
    int running; // 新增加的一行
    stPoll_t *poll;
} stGlobal_t;
```

对应的，初始化的代码也是需要修改的，在文件`src/core/base.cc`的`init_stPoll`函数里面：

```cpp
if (StudyG.poll->epollfd  < 0)
{
    stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    free(StudyG.poll);
    StudyG.poll = NULL; // 新增的一行
    return -1;
}

StudyG.poll->ncap = 16;
size = sizeof(struct epoll_event) * StudyG.poll->ncap;
StudyG.poll->events = (struct epoll_event *) malloc(size);
StudyG.poll->event_num = 0; // 新增的一行
memset(StudyG.poll->events, 0, size);
```

在`st_event_init`里面：

```cpp
if (!StudyG.poll)
{
    init_stPoll();
}

StudyG.running = 1; // 新增的一行

return 0;
```

然后，在释放的函数里面也是需要修改的，在函数`free_stPoll`里面：

```cpp
int free_stPoll()
{
    if (close(StudyG.poll->epollfd) < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    free(StudyG.poll->events);
    StudyG.poll->events = NULL;
    free(StudyG.poll);
    StudyG.poll = NULL;
    return 0;
}
```

最后，我们修改事件等待函数`st_event_wait`：

```cpp
int st_event_wait()
{
    uv_loop_t *loop = uv_default_loop();

    st_event_init();

    while (StudyG.running > 0)
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

        if (uv__next_timeout(loop) < 0 && StudyG.poll->event_num == 0)
        {
            StudyG.running = 0;
        }
    }

    st_event_free();

    return 0;
}
```

然后，我们需要在每次调用`Socket::wait_event`给事件数`+1`，修改代码：

```cpp
ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
ev->data.u64 = touint64(sockfd, id);
epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_ADD, sockfd, ev);
(StudyG.poll->event_num)++; // 新增的一行
```

`OK`，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

重新运行脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(3)
int(2)
int(4)
~/codeDir/cppCode/study #
```

我们发现，程序可以正常的结束了。这里，细心的小伙伴会发现，我在`st_event_wait`增加了代码：

```cpp
st_event_init();
```

也就是我们每次调用`st_event_wait`的时候都会尝试去进行初始化的操作。也就意味这我们可以循环的去调用这个`st_event_wait`函数了。我们编写测试脚本：

```php
<?php

study_event_init();

while (1) {
    Sgo(function () {
        var_dump(Sco::getCid());
        Sco::sleep(1);
        var_dump(Sco::getCid());
    });
    study_event_wait();
}

```

执行测试脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(1)
int(2)
int(2)
int(3)
int(3)
int(4)
int(4)
int(5)
int(5)
int(6)
int(6)
int(7)
int(7)
int(8)
^C
~/codeDir/cppCode/study #
```

我们发现，我们可以在循环里面去调用这个`st_event_wait`了。成功的解决了[《PHP扩展开发》-协程-错误使用协程库导致的Bug（一）](./《PHP扩展开发》-协程-错误使用协程库导致的Bug（一）.md)中提到的问题。

[下一篇：修复一些bug（九）](./《PHP扩展开发》-协程-修复一些bug（九）.md)
