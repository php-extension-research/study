# 协程化服务器（三）

这一篇文章，我们来协程化我们的接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, recv)
```

我们修改代码如下：

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

    zend_string *buf = zend_string_alloc(length, 0);

    Socket conn(fd);
    ret = conn.recv(ZSTR_VAL(buf), length);
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
    ZSTR_VAL(buf)[ret] = '\0';
    RETURN_STR(buf);
}
```

我们为这个`connfd`创建一个`Socket`对象。然后调用它的`recv`方法。

现在，我们去实现一下构造函数：

```cpp
Socket::Socket(int fd)
{
    sockfd = fd;
    stSocket_set_nonblock(sockfd);
}
```

然后，我们重新编译、安装扩展：

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
        var_dump($msg);
    }
});

Sco::scheduler();
```

然后启动服务器：

```shell
~/codeDir/cppCode/study # php test.php
```

然后启动客户端连接：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080

```

发送消息：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello

```

此时，服务器端打印：

```shell
~/codeDir/cppCode/study # php test.php
string(65536) "hello
"
```

我们再次发现消息给服务器：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
world

```

此时服务端报错：

```shell
Warning: Study\Coroutine\Server::recv(): recv error in /root/codeDir/cppCode/study/test.php on line 9
bool(false)

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 69632 bytes) in /root/codeDir/cppCode/study/test.php on line 9
~/codeDir/cppCode/study #
```

因为我们的服务器脚本写错了，修改如下：

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
            var_dump($msg);
        }
    }
});

Sco::scheduler();
```

重新启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后用客户端连接它，然后连续发送两次数据：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
hello
world

```

此时服务器端打印：

```shell
~/codeDir/cppCode/study # php test.php
string(65536) "hello
"
string(65536) "world
"
```

符合预期。

[下一篇：协程化服务器（四）](./《PHP扩展开发》-协程-协程化服务器（四）.md)
