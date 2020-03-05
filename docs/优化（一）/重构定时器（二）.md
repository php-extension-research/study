# 重构定时器（二）

这篇文章，我们用我们实现完的定时器替换掉`libuv`的定时器。

在文件`src/core/base.cc`里面增加如下代码：

```cpp
#include "timer.h"

using study::Timer;
using study::TimerManager;
using study::timer_manager;
```

然后修改`st_event_wait`的代码：

```cpp
int st_event_wait()
{
    st_event_init();

    while (StudyG.running > 0)
    {
        int n;
        uint64_t timeout;
        epoll_event *events;

        timeout = timer_manager.get_next_timeout();
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

        timer_manager.run_timers();

        if (timer_manager.get_next_timeout() < 0 && StudyG.poll->event_num == 0)
        {
            StudyG.running = 0;
        }
    }

    st_event_free();

    return 0;
}
```

可以发现，我们的代码反而变得更加的清晰易懂的。

然后，我们修改文件`src/coroutine/coroutine.cc`里面的代码。
增加：

```cpp
#include "timer.h"
```

然后修改`sleep_timeout`的代码：

```cpp
static void sleep_timeout(void *param)
{
    ((Coroutine *) param)->resume();
}
```

然后修改`Coroutine::sleep`这个方法：

```cpp
int Coroutine::sleep(double seconds)
{
    Coroutine* co = Coroutine::get_current();

    timer_manager.add_timer(seconds * Timer::SECOND, sleep_timeout, (void*)co);

    co->yield();
    return 0;
}
```

可以发现，这个方法也变简单了。

最后，我们删除和`libuv`有关的代码。

在文件`src/core/base.cc`里面

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

然后，我们再删除所有的`uv.h`。

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

`OK`，符合预期，我们的历史包袱算是处理完了。

现在，我们写一个简单的测试脚本：

```php
<?php

study_event_init();

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});
study_event_wait();
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(2)
int(3)
int(1)
int(2)
int(3)
~/codeDir/cppCode/study #
```

符合预期。

下一篇文章，我们来更进一步的压测，感受一下我们优化后的成果。

[下一篇：保存PHP栈](./《PHP扩展开发》-协程-保存PHP栈.md)
