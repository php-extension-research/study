# 修复一些bug（四）

开发到这里，我们花一些篇幅来对代码进行优化，毕竟我们的代码只是`it works`。

我们编写测试脚本：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        $msg = $serv->recv($connfd);
        $serv->send($connfd, $msg);
    }
});

Sco::scheduler();
```

然后启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后启动客户端：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080

```

然后发送消息：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello

```

此时，服务器是正常的：

```shell
~/codeDir/cppCode/study # php test.php

```

客户端再次发送消息：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello
world

```

此时，客户端这边没有收到`world`字符串，并且，服务器报错了：

```shell
Warning: Study\Coroutine\Server::recv(): recv error in /root/codeDir/cppCode/study/test.php on line 9
[2019-09-05 07:31:12]WARNING stSocket_set_nonblock: Error has occurred: (errno 9) Bad file descriptor in /root/codeDir/cppCode/study/src/socket.cc on line 116.
[2019-09-05 07:31:12]WARNING stSocket_send: Error has occurred: (errno 9) Bad file descriptor in /root/codeDir/cppCode/study/src/socket.cc on line 105.

Warning: Study\Coroutine\Server::send(): send error in /root/codeDir/cppCode/study/test.php on line 10

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 69632 bytes) in /root/codeDir/cppCode/study/test.php on line 9
~/codeDir/cppCode/study #
```

这里有两个问题，第一个是：

```
Error has occurred: (errno 9) Bad file descriptor
```

我们来分析一下。

出问题的地方我们可以明确就是这段：

```shell
while (1)
{
    $connfd = $serv->accept();
    $msg = $serv->recv($connfd);
    $serv->send($connfd, $msg);
}
```

当我们的服务器启动的时候，此时没有客户端连接过来，所以我们的协程会在：

```php
$connfd = $serv->accept();
```

这一行被`yield`。

当我们通过客户端连接到服务器并且发送第一条消息`hello`的时候，我们的服务器很顺利的完成了`accept`、`recv`、`send`。然后此时，协程开始第二次执行代码：

```php
$connfd = $serv->accept();
```

此时，因为没有新的连接，所以我们的协程被`yield`出去了。然后，当我们发送第二次数据的时候，此时`connfd`上面的事件被触发了，然后协程被我们的调度器`resume`执行了。然后，我们来看看`$serv->accept()`的源码：

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

当协程被`resume`之后，协程开始执行第二次：

```cpp
connfd = stSocket_accept(sockfd);
```

但是，我们现在是没有连接到来的，这个函数调用后肯定是返回了`-1`。然后，我们的服务器实际上是在`recv`这个错误的连接。我们可以来看看，修改服务器代码：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        var_dump($connfd);
        $msg = $serv->recv($connfd);
        $serv->send($connfd, $msg);
    }
});

Sco::scheduler();
```

启动服务器之后，我们用客户端给服务器发送两次消息，我们会发现服务器这端打印出了：

```shell
Warning: Study\Coroutine\Server::send(): send error in /root/codeDir/cppCode/study/test.php on line 11
int(-1)

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 69632 bytes) in /root/codeDir/cppCode/study/test.php on line 10
~/codeDir/cppCode/study #
```

符合我们分析的结果。所以，这个服务器如果要接收多次客户端的消息，应该这样写：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        while (1)
        {
            $msg = $serv->recv($connfd);
            $serv->send($connfd, $msg);
        }
    }
});

Sco::scheduler();
```

为了防止协程阻塞在了`$serv->accept();`这个地方，我们应该用一个循环来包住`recv`和`send`。

我们启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后用客户端连接，发送多次消息：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello
world
world
codinghuang
codinghuang

```

我们发现，现在没有问题了，因为加上循环之后，协程只会在`recv`以及`send`这两个地方被`yield`了。我们再来看看服务器端：

```shell
~/codeDir/cppCode/study # php test.php

```

没有任何报错。

但是，因为我们加上了一层循环，我们是无法接收其他客户端的连接。我们另起一个终端测试一下：

```shell
~/codeDir/cppCode/examples # nc 127.0.0.1 8080
hello
~/codeDir/cppCode/examples #
```

我们再来看看服务器端的输出：

```shell
Warning: Study\Coroutine\Server::recv(): recv error in /root/codeDir/cppCode/study/test.php on line 11

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 69632 bytes) in /root/codeDir/cppCode/study/test.php on line 11
~/codeDir/cppCode/study #
```

又是`recv`错误了。我们来分析一下代码：

```php
while (1)
{
    $connfd = $serv->accept();
    while (1)
    {
        $msg = $serv->recv($connfd);
        $serv->send($connfd, $msg);
    }
}
```

当我们的第一个客户端不再继续给服务器发送消息的时候，这个协程就会在`recv`处被`yield`。然后，当来了一个新的客户端进行连接的时候，我们的协程又回被`resume`。我们来看看`recv`对应的源码：

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

当协程被`resume`的时候，协程会执行第二次的：

```cpp
ret = stSocket_recv(sockfd, buf, len, 0);
```

因为我们第一个客户端是没有发送数据的，所以，这里肯定是读取不到数据的。所以这里会报错`recv error`。实战到这里，大家有没有发现，我这个服务器实际上很奇怪？

我们的协程因为`accept`被`yield`，但是却因为数据可以`recv`了，又被`resume`了。或者说因为`recv`被`yield`了，又因为一个新的客户端连接而被`resume`了。我们期待的应该是因为`accept`被`yield`了，就只能因为可以`accept`了才被`resume`。或者说因为`recv`被`yield`了，就只能因为可以`recv`了才被`resume`。所以，我们需要修改我们的`Socket`操作被阻塞的逻辑。因为篇幅原因，我们另起一篇文章讲解。

[下一篇：修复一些bug（五）](./《PHP扩展开发》-协程-修复一些bug（五）.md)
