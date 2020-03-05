# 协程化Socket::Socket

看你有小伙伴们会问，创建一个`socket`为什么需要协程化？对于这个问题，我们得先理解我们的协程是如何判断某个`socket`不可以进行读写。

是这样的，我们在实现这个功能的时候，利用到了`socket`这个资源不可以使用的时候，会返回一个`EAGAIN`错误码。并且，我们在去试探这个`socket`是否可以读写的时候，是不可以被阻塞的，因为我们的协程一旦被阻塞，就会失去对CPU的控制权。所以，我们需要在创建`socket`的时候，设置这个`socket`为非阻塞的。

我们在`src/coroutine/socket.cc`里面编写如下代码：

```cpp
Socket::Socket(int domain, int type, int protocol)
{
    sockfd = stSocket_create(domain, type, protocol);
    if (sockfd < 0)
    {
        return;
    }
    stSocket_set_nonblock(sockfd);
}
```

考虑到我们将来会把这个扩展底层的`Socket`类作为借口提供给应用层，所以，我们把原来的函数：

```cpp
int stSocket_create(int type)
```

重构为了：

```cpp
int stSocket_create(int domain, int type, int protocol)
```

让用户可以更加的灵活控制参数。

然后，我们需要去修改文件`include/socket.h`里面这个函数的声明，变成：

```cpp
int stSocket_create(int domain, int type, int protocol);
```

然后重新实现这个函数：

```cpp
int stSocket_create(int domain, int type, int protocol)
{
    int sock;

    sock = socket(domain, type, protocol);
    if (sock < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }

    return sock;
}
```

然后，我们需要去实现函数`stSocket_set_nonblock`。

我们现在文件`include/socket.h`里面声明这个函数：

```cpp
int stSocket_set_nonblock(int sock);
```

然后在文件`src/socket.cc`里面实现：

```cpp
int stSocket_set_nonblock(int sock)
{
    int flags;

    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return -1;
    }
    flags = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}
```

其中，

```cpp
flags = fcntl(sock, F_GETFL, 0);
```

用来获取这个`socket`原来的一些属性。

```cpp
flags = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
```

用来在原来的属性上加上非阻塞的属性。

现在，我们需要去修改我们创建服务器的代码，因为那里用到了函数`stSocket_create`。在接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
```

里面，我们修改代码如下：

```cpp
sock = stSocket_create(AF_INET, SOCK_STREAM, 0); // 修改的地方
stSocket_bind(sock, ST_SOCK_TCP, Z_STRVAL_P(zhost), zport);
stSocket_listen(sock);
```

然后，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
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

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

while (1)
{
    $connfd = $serv->accept();
    while (1)
    {
        $buf = $serv->recv($connfd);
        if ($buf == false)
        {
            break;
        }
        $serv->send($connfd, "hello");
    }
}
```

启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 

```

然后另开一个终端去连接这个服务器并且发送消息：

```shell
~/codeDir/cppCode # nc 127.0.0.1 8080
codinghuang
hello
```

成功的收到了服务器发来的`hello`字符串。符合预期。

这里，我们并没有马上使用协程化的`coroutine::Socket`来创建套接字，因为我们还有其他的方法没有实现，所以还是用不了。我们必须全部协程化之后，才可以使用。所以，我们这里只需要保证非协程化的`socket`函数可用即可。

[下一篇：实现coroutine::Socket::bind和listen](./《PHP扩展开发》-协程-实现coroutine::Socket::bind和listen.md)

