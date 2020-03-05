# 修复一些bug（六）

现在，我们来修复一个内存泄漏的问题。我们编写测试脚本：

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

这个服务器是我们上一篇文章写过的。我们启动这个服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后用一个客户端去连接它：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080

```

然后客户端直接断开连接：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test #
```

我们会在服务器这端看到如下报错：

```shell
~/codeDir/cppCode/study # php test.php

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 69632 bytes) in /root/codeDir/cppCode/study/test.php on line 11
~/codeDir/cppCode/study #
```

说是内存耗尽了对吧，而且很显然是`PHP`报错的对吧。那也就意味着我们在某个地方使用了`PHP`封装的分配内存的函数，然后没有去释放这个内存。我们来分析一下我们的脚本，问题显然是出在了循环的那个位置，因为只有这里才可能耗尽我们的内存：

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

首先`accept`这个地方肯定是不会造成这个报错的，因为我们没有去分配大片的内存。然后我们来看看`recv`的源码，我们会发现这个地方分配了内存，并且是在堆上面，也就意味着我们需要自己手动去释放这篇内存，但是，我们忘记去释放了对吧（这也体现出了常驻内存的程序的坏处，你必须确保你的代码质量很高，不会有内存泄漏。如果是我们原来的`FPM`模式下，是不会造成内存泄漏的，所以，技术不能够盲从，需要选择**合适的**）：

```cpp
zend_string *buf = zend_string_alloc(length, 0);
```

因为我们知道，客户端断开连接的时候，协程会被`resume`，`recv`函数实际上是会执行的，并且`C`底层的`recv`会返回`0`，所以，这就导致了我们的`recv`函数一直在循环执行，但是我们却没有释放掉在`recv`接口里面分配的内存，所以造成内存泄漏。这里，我们可以`recv`接口里面去释放掉这块`buf`内存，但是，我们可以用一个比较简单的方法解决这个问题，而且还可以避免循环的内存分配。

我们知道，一个线程里面同时只会有一个协程占用`CPU`，如果我们不手动切换这个协程的化，一定会等到这个协程执行完毕对吧。所以，我们可以定义一个全局的`buf`，然后所有的协程都使用这一个`buf`来发送数据。并且，协程之间还不会抢占这个`buf`（这里体现出了协程比线程方便的一个优势，在多线程下面，我们是需要对这个全局的`buf`加锁的）。

我们现在来定义一下这个全局的`buf`，在文件`include/coroutine_socket.h`里面：

```cpp
public:
    static char *read_buffer;
    static size_t read_buffer_len;

    static char *write_buffer;
    static size_t write_buffer_len;
```

然后在文件`src/coroutine/socket.cc`里面进行初始化：

```cpp
using study::Coroutine;
using study::coroutine::Socket;

char * Socket::read_buffer = nullptr; // 增加的地方
size_t Socket::read_buffer_len = 0; // 增加的地方
char * Socket::write_buffer = nullptr; // 增加的地方
size_t Socket::write_buffer_len = 0; // 增加的地方
```

然后，我们需要定义一个分配内存的方法，首先文件`include/coroutine_socket.h`里面定义方法：

```cpp
public:
    static int init_read_buffer();
    static int init_write_buffer();
```

然后，在文件`src/coroutine/socket.cc`里面进行实现：

```cpp
int Socket::init_read_buffer()
{
    if (!read_buffer)
    {
        read_buffer = (char *)malloc(65536);
        if (read_buffer == NULL)
        {
            return -1;
        }
        read_buffer_len = 65536;
    }

    return 0;
}

int Socket::init_write_buffer()
{
    if (!write_buffer)
    {
        write_buffer = (char *)malloc(65536);
        if (write_buffer == NULL)
        {
            return -1;
        }
        write_buffer_len = 65536;
    }

    return 0;
}
```

然后，我们修改我们`recv`的接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, recv)
{
    ssize_t ret;
    zend_long fd;
    zend_long length = 65536;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(fd)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket::init_read_buffer();

    Socket conn(fd);
    ret = conn.recv(Socket::read_buffer, Socket::read_buffer_len);
    if (ret == 0)
    {
        zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("errCode"), ST_ERROR_SESSION_CLOSED_BY_CLIENT);
        zend_update_property_string(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("errMsg"), st_strerror(ST_ERROR_SESSION_CLOSED_BY_CLIENT));
        RETURN_FALSE;
    }
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "recv error");
        RETURN_FALSE;
    }
    Socket::read_buffer[ret] = '\0';
    RETURN_STRING(Socket::read_buffer);
}
```

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

然后，我们重新启动我们的服务器脚本：

```shell
~/codeDir/cppCode/study # php test.php

```

然后，用一个客户端去连接它，并且直接断开连接：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test #
```

我们发现，服务器端没有挂掉：

```shell
~/codeDir/cppCode/study # php test.php

```

符合预期。

[下一篇：server关闭连接](./《PHP扩展开发》-协程-server关闭连接.md)
