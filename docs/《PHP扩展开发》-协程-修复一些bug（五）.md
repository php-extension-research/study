# 修复一些bug（五）

这篇文章，我们来修复一下上一篇文章提到的问题。首先，我们来修复一下我们的`Socket::accept`方法：

```cpp
int Socket::accept()
{
    int connfd;

    do
    {
        connfd = stSocket_accept(sockfd);
    } while (connfd < 0 && errno == EAGAIN && wait_event(ST_EVENT_READ));

    return connfd;
}

ssize_t Socket::recv(void *buf, size_t len)
{
    int ret;

    do
    {
        ret = stSocket_recv(sockfd, buf, len, 0);
    } while (ret < 0 && errno == EAGAIN && wait_event(ST_EVENT_READ));

    return ret;
}

ssize_t Socket::send(const void *buf, size_t len)
{
    int ret;

    do
    {
        ret = stSocket_send(sockfd, buf, len, 0);
    } while (ret < 0 && errno == EAGAIN && wait_event(ST_EVENT_WRITE));

    return ret;
}
```

代码都是类似的，都是把尝试`accept`、`recv` 、`send`的操作用一个`while`循环去包住它。这样，就不会因为其他的事件到来，协程在错误的地方执行代码了。

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

编写测试脚本：

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

然后期待服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后，我们用同一个客户端发送两次数据：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello
world

```

此时服务器端是正常的了：

```shell
~/codeDir/cppCode/study # php test.php

```

至于这里客户端为什么不会打印出`world`，是因为我们的服务器编写错误。我们后面会纠正这个服务器。

我们现在来编写第二个测试代码：

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

然后期待服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

接着，我们用一个客户端去连接服务器，并且发送多条数据：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello
world
world
codinghuang
codinghuang

```

此时，服务器端是正常的：

```shell
~/codeDir/cppCode/study # php test.php

```

然后，我们再次开启一个客户端去连接这个服务器：

```shell
~/codeDir/cppCode/examples # nc 127.0.0.1 8080

```

此时，服务器端已经不再报错了：

```shell
~/codeDir/cppCode/study # php test.php

```

符合我们的预期。

现在，我们回到刚才的问题，为什么第一个测试脚本，我们用一个客户端连续发送两次消息，第二个消息没有被返回呢？我们再来看看脚本：

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

我们发现，当我们接收到请求的时候，并且`recv`和`send`到第一个消息的时候，我们的服务器再次回到了：

```php
$connfd = $serv->accept();
```

然后被`yield`了。但是，问题来了。当我们服务器收到了第二次消息之后，虽然协程会被`resume`，但是，这个事件不是客户端来了连接，所以在`accept()`里面，还会陷入`Socket::accept`的循环里面。所以，协程是到不了：

```php
$msg = $serv->recv($connfd);
```

这一行的。自然，客户端发送的第二条消息就不会被返回了。而且，这个行为是符合我们同步编程的思路的，不许先处理完前面的，才可以处理后面的。但是又要充分理解为什么说协程可以达到异步的效果，因为通过协程，可以在事件触发的时候，让协程自动去执行对应的代码，这符合一种主动通知的特点。

我们需要修改我们的服务器代码：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        Sgo(function () use ($serv, $connfd)
        {
            while (1)
            {
                $msg = $serv->recv($connfd);
                $serv->send($connfd, $msg);
            }
        });
    }
});

Sco::scheduler();
```

然后用一个客户端去连接这个服务器，然后发送多条消息：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
hello
world
world
codinghuang
codinghuang

```

并且服务器端是正常的：

```shell
~/codeDir/cppCode/study # php test.php

```

然后，我们再另起一个客户端，发送多条消息：

```shell
~/codeDir/cppCode/examples # nc 127.0.0.1 8080
hello
hello
codinghuang
codinghuang

```

可以看到，消息成功的返回来了。

并且，服务器端也是正常的：

```shell
~/codeDir/cppCode/study # php test.php

```

所以，虽然通过协程，可以通过同步的方式编写高性能的服务器，但是，我们只有掌握了协程的原理，才可以避免一些坑。
