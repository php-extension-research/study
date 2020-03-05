# 实现Channel基础类

这一篇文章，我们来实现一下`src`下面的`Channel`类，为我们后续支持对应的扩展接口做准备。

首先，创建文件`include/coroutine_channel.h`，内容如下：

```cpp
#ifndef COROUTINE_CHANNEL_H
#define COROUTINE_CHANNEL_H

#include "study.h"
#include "coroutine.h"

namespace study { namespace coroutine {
class Channel
{
public:
    enum opcode
    {
        PUSH = 1,
        POP = 2,
    };

    Channel(size_t _capacity = 1);
    ~Channel();
    void* pop(double timeout = -1);
    bool push(void *data, double timeout = -1);

protected:
    size_t capacity = 1;
    bool closed = false;
    std::queue<Coroutine*> producer_queue;
    std::queue<Coroutine*> consumer_queue;
    std::queue<void*> data_queue;
};
}
}

#endif  /* COROUTINE_CHANNEL_H */
```

其中，

```cpp
enum opcode
{
    PUSH = 1,
    POP = 2,
};
```

`opcode`代表对这个`Channel`进行何种操作，是`push`还是`pop`。

```cpp
Channel(size_t _capacity = 1);
```

是构造函数，`Channel`默认大小是`1`。

```cpp
void* pop(double timeout = -1);
bool push(void *data, double timeout = -1);
```

代表对`Channel`进行`pop`和`push`操作。

```cpp
std::queue<Coroutine*> producer_queue;
std::queue<Coroutine*> consumer_queue;
```

用来存储处于等待`Channel`的生产者协程和消费者协程。

```cpp
std::queue<void*> data_queue;
```

存储`Channel`使用到的数据。

接着，我们去实现这些方法，创建文件`src/coroutine/channel.cc`。然后记得修改`config.m4`文件，增加这个文件需要被编译，然后重新生成一份`Makefile`。接着，去实现对应的方法。

首先，引入头文件：

```cpp
#include "coroutine.h"
#include "coroutine_channel.h"
#include "timer.h"

using study::Coroutine;
using study::coroutine::Channel;
using study::Timer;
using study::TimerManager;
using study::timer_manager;
```

然后，实现构造函数和析构函数：

```cpp
Channel::Channel(size_t _capacity):
    capacity(_capacity)
{
}

Channel::~Channel()
{
}
```

接着，实现协程超时执行的回调函数：

```cpp
static void sleep_timeout(void *param)
{
    ((Coroutine *) param)->resume();
}
```

很简单，就是让协程恢复原来的执行状态。

接着，实现`Channel`的`pop`操作。

```cpp
void* Channel::pop(double timeout)
{
    Coroutine *co = Coroutine::get_current();
    void *data;

    if (data_queue.empty())
    {
        if (timeout > 0)
        {
            timer_manager.add_timer(timeout * Timer::SECOND, sleep_timeout, (void*)co);
        }
        consumer_queue.push(co);
        co->yield();
    }

    if (data_queue.empty())
    {
        return nullptr;
    }

    data = data_queue.front();
    data_queue.pop();

    /**
     * notice producer
     */
    if (!producer_queue.empty())
    {
        co = producer_queue.front();
        producer_queue.pop();
        co->resume();
    }

    return data;
}
```

其中，

```cpp
if (data_queue.empty())
{
    if (timeout > 0)
    {
        timer_manager.add_timer(timeout * Timer::SECOND, sleep_timeout, (void*)co);
    }
    consumer_queue.push(co);
    co->yield();
}
```

我们先判断有没有数据，如果没有数据，并且设置了协程超时等待的数据的时间，那么，我们就设置一个定时器。然后，把这个协程加入到消费者等待队列里面，最后，`yield`这个协程。

```cpp
if (data_queue.empty())
{
    return nullptr;
}
```

这段代码实际上是专门为`timeout`准备的。因为协程`timeout`恢复运行的时候，`Channel`可能还是没有数据，所以我们需要这一步判断。

```cpp
data = data_queue.front();
data_queue.pop();

/**
 * notice producer
 */
if (!producer_queue.empty())
{
    co = producer_queue.front();
    producer_queue.pop();
    co->resume();
}

return data;
```

取出`Channel`里面的数据，然后如果有生产者协程在等待，那么就`resume`那个生产者协程。最后，等生产者协程执行完毕，或者生产者协程主动`yield`，才会回到消费者协程，最后返回`data`。

接着，实现`Channel`的`push`操作：

```cpp
bool Channel::push(void *data, double timeout)
{
    Coroutine *co = Coroutine::get_current();
    if (data_queue.size() == capacity)
    {
        if (timeout > 0)
        {
            timer_manager.add_timer(timeout * Timer::SECOND, sleep_timeout, (void*)co);
        }
        producer_queue.push(co);
        co->yield();
    }

    /**
     * channel full
     */
    if (data_queue.size() == capacity)
    {
        return false;
    }

    data_queue.push(data);

    /**
     * notice consumer
     */
    if (!consumer_queue.empty())
    {
        co = consumer_queue.front();
        consumer_queue.pop();
        co->resume();
    }

    return true;
}
```

代码和`pop`类似，所以不多啰嗦。

`OK`，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

符合预期。

[下一篇：Channel的push和pop](./《PHP扩展开发》-协程-Channel的push和pop.md)
