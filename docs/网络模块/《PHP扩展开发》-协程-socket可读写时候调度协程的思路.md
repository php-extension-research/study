# socket可读写时候调度协程的思路

这篇文章，我们来就先不写代码了，我们先来理清一下`socket`可读写的时候如何调度对应的协程。

首先，我们需要一种可以判断`socket`可以读写的方法。我们这里采用`epoll`这个东西，它可以在事件来的时候通知我们的线程。这样，我们就知道了哪些`socket`是可以读或写的。

因为我们需要唤醒对应的协程，那么，我们得到了某个`socket`可以读写，怎么判断这个`socket`是被哪个协程管理的呢？这里，我们使用了一个小技巧，有如下一段代码可以把`socket fd`和`coroutine id`联系起来。

我们先把`fd`和`id`两个`int`合成一个整数：

```cpp
uint64_t touint64(int fd, int id)
{
    uint64_t ret = 0;
    ret |= ((uint64_t)fd) << 32;
    ret |= ((uint64_t)id);

    return ret;
}
```

然后再反解出来：

```cpp
void fromuint64(uint64_t v, int *fd, int *id)
{
    *fd = (int)(v >> 32);
    *id = (int)(v & 0xffffffff);
}
```

这里，我们通过`v`这个整数，就可以得到`fd`和`id`了。

于是，就有了如下伪代码可以完成`socket`不可以读写时候的协程`yield`：

```cpp
if 协程不可以读socket
{
    wait_event(socket, ST_FD_READ);
}

if 协程不可以写socket
{
    wait_event(socket, ST_FD_WRITE);
}

wait_event(int fd, int flag)
{
    Coroutine* co = Coroutine::get_current();
    long id = co->get_cid();

    ev.events = flag == ST_FD_READ ? EPOLLIN : EPOLLOUT;
    ev.data.u64 = touint64(fd, id);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);

    co->yield();
}
```

代码很简单，首先我们判断当前这个`socket`是否可以读写，如果不可以，那么调用`wait_event`。`wait_event`做的事情很简单，现获取当前的协程，然后再调用`touint64`把`socket`和`co_id`两个结合其他，再通过`epoll_ctl`添加事件，最后，`yield`当前协程。

那么，我们如何在这个`socket`可以读写的时候`resume`这个协程呢？伪代码如下：

```cpp
int PHPCoroutine::scheduler()
{
    // 省略了之前的代码
  
    n = epoll_wait(epollfd, events, ncap, 0);
    for (i = 0; i < n; i++) {
        int fd;
        int cid;
        struct epoll_event *p = &events[i];
        uint64_t u64 = p->data.u64;

        fromuint64(u64, &fd, &cid);
        Coroutine* co = Coroutine::get_by_cid(cid);
        co->resume();
    }
}
```

这段代码比较简单，就是通过`epoll_wait`来得知事件的到来，然后遍历这些事件，通过`fromuint64`得到`socket`以及`cid`，然后再`resume`这个协程。此时，协程就可以继续完成之前读或写`socket`的操作了。

（这里，我们会发现没有用上反解出来的`fd`，但是，为了防止以后会用到，所以我们有了这种把`fd`和`cid`结合起来的操作）

`OK`，我们后面的章节就会根据这个思路来实现我们的代码，以协程化我们的`accept`、`recv`、`send`函数。

[下一篇：全局变量STUDYG](./《PHP扩展开发》-协程-全局变量STUDYG.md)

