# 重构定时器

这一篇文章，我们来重构一下定时器。我们不使用`libuv`的定时器，而是自己去实现一个定时器。为什么要这么做呢？这里我很有必要花一些时间给大家说明一下。如果我们在循环里面去调用`Co::sleep`函数，会出现一些问题，或许在`PHP`层面上不会报错，但是，把`src`抽离出来，用`C++`代码做测试，就会发现这个问题。而且，如果我们的`timer`在堆上面分配内存的话，那么是不能够正常的释放`timer`的内存的，必须使用`libuv`的`uv_close`函数，在`uv_close`中的回调函数里面来释放，这就很头痛了。因为回调函数只能够通过`libuv`的那套事件驱动来被调用。但是，我们没有用到`libuv`的那套事件驱动，所以释放`timer`的回调函数就无法被调用，因此就会造成内存泄漏。这个问题花了我`1`天的时间去调试。主要是对`libuv`不熟悉，我还是在大佬的帮助下发现的这个问题。那么，为什么不使用`libuv`的那套事件驱动呢？因为，如果我们直接使用它的事件驱动，那么我们会失去很多学习的机会，而且我们的代码也会一直被`libuv`束缚住，这是我不希望看到的，所以，我决定自己去实现一个定时器，而且这个难度不是很大，因为`C++`有很多实用的容器，可以助力我们实现定时器。

我们来想一下我们的这个定时器必须要实现的功能：

```shell
1、创建一个定时器（可以设置超时的时间，以及超时之后被回调的函数）
2、获取当前的时间（因为，我们需要用定时器里面的时间和当前时间进行比较，如果到期，说明需要执行回调函数）
3、最接近过期时间的优先被执行（因此，我们可以使用优先队列实现一个最小堆的效果）
4、获取下一个过期的时间（得到的这个时间作为`epoll_wait`的阻塞时间）
5、检查是否有到期的定时器（如果到期，那么执行设置好的回调函数）
```

然后，我们创建两个文件`include/timer.h`和`src/timer.cc`，记得修改`config.m4`文件，增加需要编译的文件`src/timer.cc`，并且重新生成一份`Makefile`。

`timer.h`里面的内容如下：

```cpp
#ifndef TIMER_H
#define TIMER_H

#include "study.h"

typedef void (*timer_func_t)(void*);

namespace study
{
class TimerManager;

class Timer
{
friend class TimerManager;
friend class CompareTimerPointer;
public:
    static const uint64_t MILLI_SECOND;
    static const uint64_t SECOND;
    Timer(uint64_t _timeout, timer_func_t _callback, void *_private_data, TimerManager *_timer_manager);
    static uint64_t get_current_ms();

private:
    uint64_t timeout = 0;
    uint64_t exec_msec = 0;
    timer_func_t callback;
    void *private_data;
    TimerManager *timer_manager = nullptr;
};

class CompareTimerPointer
{
public:
    bool operator () (Timer* &timer1, Timer* &timer2) const
    {
        return timer1->exec_msec > timer2->exec_msec;
    }
};

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();
    void add_timer(int64_t _timeout, timer_func_t _callback, void *_private_data);
    int64_t get_next_timeout();
    void run_timers();
private:
    std::priority_queue<Timer*, std::vector<Timer*>, CompareTimerPointer> timers;
};

extern TimerManager timer_manager;
}

#endif /* TIMER_H */

```

我们先来看看`Timer`这个类的成员方法。

其中，

`Timer`类是定时器。

```cpp
friend class TimerManager;
```

是用来声明`TimerManager`类是`Timer`的一个"朋友"。这样的话，`TimerManager`类就可以访问`Timer`类的成员变量了。

```cpp
Timer(uint64_t _timeout, timer_func_t _callback, void *_private_data, TimerManager *_timer_manager);
```

是构造函数，`_timeout`表示`_timeout`毫秒之后定时器过期。`_callback`表示定时器过期之后需要调用的回调函数。`_private_data`是传递给这个回调函数的参数。`_timer_manager`是这个`Timer`对应的管理器。

```cpp
static uint64_t get_current_ms();
```

获取当前的时间戳，单位是毫秒`ms`。

我们再来看看`Timer`这个类的成员变量：

`timeout`是定时器过期的时间，也就是定时多久，例如`1ms`这种。

`exec_msec`是用来判断定时器是否过期的那个时间点，单位也是`ms`。
