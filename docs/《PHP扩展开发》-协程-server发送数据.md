# server发送数据

这篇文章，我们来实现一下服务器发送数据的逻辑。也就是说，我们需要实现如下接口：

```php
Study\Coroutine\Server->send(int fd, string data): int | false;
```

首先，我们需要定义接口的参数，在文件`study_server_coro.cc`里面：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_server_coro_send, 0, 0, 2)
    ZEND_ARG_INFO(0, fd)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()
```

然后实现`send`接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, send)
{
    ssize_t retval;
    zend_long fd;
    char *data;
    size_t length;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(fd)
        Z_PARAM_STRING(data, length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    retval = stSocket_send(fd, data, length, 0);
    if (retval < 0)
    {
        php_error_docref(NULL, E_WARNING, "send error");
        RETURN_FALSE;
    }
    RETURN_LONG(retval);
}
```

`stSocket_send`是用来发送数据的，我们在文件`include/socket.h`里面进行声明：

```cpp
ssize_t stSocket_send(int sock, void *buf, size_t len, int flag);
```

然后在文件`src/socket.cc`里面进行实现：

```cpp
ssize_t stSocket_send(int sock, void *buf, size_t len, int flag)
{
    ssize_t ret;

    ret = send(sock, buf, len, flag);
    if (ret < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    return ret;
}
```

最后，我们需要注册这个方法：

```cpp
static const zend_function_entry study_coroutine_server_coro_methods[] =
{
    // 省略其他注册的方法
    PHP_ME(study_coroutine_server_coro, send, arginfo_study_coroutine_server_coro_send, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

`OK`，我重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

$sock = $serv->accept();
$buf = $serv->recv($sock);
$serv->send($sock, $buf);
```

脚本很简单，就是把客户端发过来的数据原封不动的发送给客户端，执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 

```

然后另起一个终端，连接服务器之后发送`hello world`：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello world
hello world
```

`OK`，服务器发送成功。

[下一篇：server错误码](./《PHP扩展开发》-协程-server错误码.md)

