# 重构协程调度器模块

这篇文章，我们重构一下协程调度器，也就是：

```cpp
int PHPCoroutine::scheduler()
```

首先，我打算把调度器的模块实现在更加的低一层次，也就是把代码放在`src`目录下面，因为如果以后，我们需要把`src`编译成动态链接库，那我们就不会丢失这个调度器的代码（如果大家现在还不理解这段话，不要紧，以后你们会明白为什么了）

并且，把调度器换一个名字，我们叫做`st_event_init()`。因为，以后我们的调度器除了调度功能，还会有很多其他的功能，例如定时器功能，但是定时器功能是和协程调度无关的，所以我们需要修改这个名字。并且，我们把这个新的事件函数放在文件`src/core/base.cc`里面去实现。

`OK`我们来修改代码。我们先声明一些基本的函数，在文件`include/study.h`里面：

```cpp
int init_stPoll();
int free_stPoll();

int st_event_init();
int st_event_wait();
int st_event_free();
```

然后我们删除`study.h`里面实现的代码，我们在文件`src/core/base.cc`里面去实现它们：

```cpp
static inline void init_stPoll()
static inline void free_stPoll()
```

然后，我们在文件`src/core/base.cc`里面引入头文件：

```cpp
#include "uv.h"
#include "coroutine.h"
#include "log.h"

using study::Coroutine;
```

然后，我们在文件`src/core/base.cc`里面实现`init_stPoll`函数：

```cpp
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
```

代码很简单，实际上就和我们原来的`init_stPoll`一样，只不过做了一些返回值判断。

然后，我们再来实现`free_stPoll`：

```cpp
int free_stPoll()
{
    free(StudyG.poll->events);
    free(StudyG.poll);
    return 0;
}
```

代码很简单，就是去释放从堆中分配的内存。

接下来，我们去实现函数`st_event_init`：

```cpp
int st_event_init()
{
    if (!StudyG.poll)
    {
        init_stPoll();
    }

    return 0;
}
```

代码很简单，就是去调用`init_stPoll`来初始化我们的`StudyG.poll`。

然后，我们实现函数`st_event_wait`：

```cpp
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
```

**注意，这里我用到了stError这个宏，大家需要自己去这个项目代码里面拷贝一份这个宏。**

最后，我们去实现函数`st_event_free`：

```cpp
int st_event_free()
{
    free_stPoll();
    return 0;
}
```

然后我们删除文件`study_coroutine.cc`重复的代码：

```cpp
typedef enum
{
    UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
    UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

extern "C" void uv__run_timers(uv_loop_t* loop);
extern "C" uint64_t uv__hrtime(uv_clocktype_t type);
extern "C" int uv__next_timeout(const uv_loop_t* loop);
```

并且，删除方法：

```cpp
int PHPCoroutine::scheduler()
```

然后删除扩展接口：

```cpp
PHP_METHOD(study_coroutine_util, scheduler)
```

然后，增加两个新的接口`study_event_init`和`study_event_wait`。

在文件`study.cc`里面：

先定义参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_void, 0, 0, 0)
ZEND_END_ARG_INFO()
```

因为这两个接口不接收任何参数。

```cpp
PHP_FUNCTION(study_event_init)
{
    int ret;
    ret = st_event_init();
    if (ret < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_FUNCTION(study_event_wait)
{
    int ret;
    ret = st_event_wait();
    if (ret < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

代码很简单，就是去调用我们刚才写好的两个函数。

然后在`study.cc`的`study_functions`里面注册这两个接口：

```cpp
PHP_FE(study_event_init, arginfo_study_coroutine_void)
PHP_FE(study_event_wait, arginfo_study_coroutine_void)
``

`OK`，我们编译代码：

```shell
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后编写测试脚本：

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

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(3)
int(2)
int(4)

```

`OK`，符合预期。
