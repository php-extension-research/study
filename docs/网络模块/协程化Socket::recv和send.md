# 协程化Socket::recv和send

这篇文章，我们来协程化：
```cpp
ssize_t recv(void *buf, size_t len);
ssize_t send(const void *buf, size_t len);
```

我们在文件`src/corotine/socket.cc`里面实现`recv`：

```cpp
ssize_t Socket::recv(void *buf, size_t len)
{
    int ret;

    ret = stSocket_recv(sockfd, buf, len, 0);
    if (ret < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_READ);
        ret = stSocket_recv(sockfd, buf, len, 0);
    }
    return ret;
}
```

代码和协程化的`accept`的类似。先**尝试**着读取数据，如果不可以读取，那么就切换出这个协程。等到事件触发（也就是可读的时候），通过调度器`resume`这个协程，然后协程继续回到这个位置，然后执行第二次`recv`。

类似的，我们实现一下`send`：

```cpp
ssize_t Socket::send(const void *buf, size_t len)
{
    int ret;

    ret = stSocket_send(sockfd, buf, len, 0);
    if (ret < 0 && errno == EAGAIN)
    {
        wait_event(ST_EVENT_WRITE);
        ret = stSocket_send(sockfd, buf, len, 0);
    }
    return ret;
}
```

思路和`recv`一致，不多啰嗦。

然后，我们重新编译、安装扩展：

```shell
/root/codeDir/cppCode/study/src/coroutine/socket.cc:63:37: error: invalid conversion from 'const void*' to 'void*' [-fpermissive]
         ret = stSocket_send(sockfd, buf, len, 0);
```

报错了。因为我们的参数类型不一致。我们需要修改`stSocket_send`函数的参数声明，在文件`include/socket.h`里面：

```cpp
ssize_t stSocket_send(int sock, const void *buf, size_t len, int flag);
```

然后，在文件`src/socket.cc`里面也需要去修改为：

```cpp
ssize_t stSocket_send(int sock, const void *buf, size_t len, int flag)
```

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

编译成功，符合预期。

[下一篇：实现coroutine::Socket::close](./《PHP扩展开发》-协程-实现coroutine::Socket::close.md)

