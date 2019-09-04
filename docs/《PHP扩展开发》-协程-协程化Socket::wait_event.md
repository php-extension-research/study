# 协程化Socket::wait_event

这篇文章，我们来协程化：
```cpp
int study::coroutine::Socket::wait_event()
```

我们在文件`src/coroutine/socket.cc`里面编写代码：

```cpp
bool Socket::wait_event(int event)
{
    long id;
    Coroutine* co;
    epoll_event *ev;

    co = Coroutine::get_current();
    id = co->get_cid();

    ev = StudyG.poll.events;

    ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
    ev->data.u64 = touint64(sockfd, id);
    epoll_ctl(StudyG.poll.epollfd, EPOLL_CTL_ADD, sockfd, ev);

    co->yield();
    return true;
}
```

其中，

```cpp
co = Coroutine::get_current();
```

获取到当前的这个协程。

```cpp
ev = StudyG.poll.events;
ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
```

用来判断这个协程需要等待那种类型的事件，目前是支持`READ`和`WRITE`。

```cpp
ev->data.u64 = touint64(sockfd, id);
```

则是设置事件到来的时候，我们需要取出的数据，这里，我们最主要的是获取协程的`id`。因为，当事件触发的时候，我们需要`resume`对应的协程。

```cpp
epoll_ctl(StudyG.poll.epollfd, EPOLL_CTL_ADD, sockfd, ev);
```

用来把事件注册到全局的`epollfd`上面。

```cpp
co->yield();
```

把当前的这个协程切换出去。

`OK`，我们现在来定义一下我们的`READ`、`WRITE`宏。在文件`include/study.h`里面：

```cpp
enum stEvent_type
{
    ST_EVENT_NULL   = 0,
    ST_EVENT_DEAULT = 1u << 8,
    ST_EVENT_READ   = 1u << 9,
    ST_EVENT_WRITE  = 1u << 10,
    ST_EVENT_RDWR   = ST_EVENT_READ | ST_EVENT_WRITE,
    ST_EVENT_ERROR  = 1u << 11,
};
```

因为我们使用了类`Coroutine`，所以，我们需要在文件`src/coroutine/socket.cc`里面去指明：

```cpp
#include "coroutine_socket.h"
#include "coroutine.h" // 增加的一行
#include "socket.h"

using study::Coroutine; // 增加的一行
using study::coroutine::Socket;
```

然后，我们去实现我们的`touint64`函数，在文件`include/study.h`里面：

```cpp
static inline uint64_t touint64(int fd, int id)
{
    uint64_t ret = 0;
    ret |= ((uint64_t)fd) << 32;
    ret |= ((uint64_t)id);

    return ret;
}
```

`OK`，我们现在重新编译、安装我们的扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

编译成功，符合预期。

[下一篇：在事件到来时resume对应的协程](./《PHP扩展开发》-协程-在事件到来时resume对应的协程.md)

