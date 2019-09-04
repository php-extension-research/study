# 协程化Socket::accept

这篇文章，我们来协程化：
```cpp
int study::coroutine::Socket::accept()
```

我们在文件`src/coroutine/socket.cc`里面编写代码：

```cpp
int Socket::accept()
{
    int connfd;

    connfd = stSocket_accept(sockfd);
    if (connfd < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_READ);
        connfd = stSocket_accept(sockfd);
    }

    return connfd;
}
```

其中，

```cpp
connfd = stSocket_accept(sockfd);
```

我们先第一次调用函数`stSocket_accept`来**尝试**获取客户端连接，如果返回值`connfd`小于`0`，并且`errno == EAGAIN`，也就是代码：

```cpp
if (connfd < 0 && errno == EAGAIN)
```

那么就说明此时没有客户端连接，我们就需要等待事件（此时的事件是有客户端的连接，这是一个**可读**的事件）的到来，并且`yield`这个协程。我们把这个等待和`yield`的操作封装在了函数`wait_event`里面。

一旦事件到来（此时，有客户端连接），我们的调度器就会去`resume`这个协程（这个调度部分我们后面会去实现它），然后，协程再次调用`stSocket_accept`，就可以获取到连接了。

然后再把获取的连接返回。

我们将会在下一篇文章实现：

```cpp
bool study::coroutine::Socket::wait_event(int event)
```

方法。