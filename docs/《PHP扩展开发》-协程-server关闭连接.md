# server关闭连接

这一篇文章，为了较好的进行服务器压测，我们来实现一下`Server`关闭连接套接字的接口。先定义接口参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_server_coro_close, 0, 0, 1)
    ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()
```

然后在`study_coroutine_server_coro_methods`里面注册接口：

```cpp
PHP_ME(study_coroutine_server_coro, close, arginfo_study_coroutine_server_coro_close, ZEND_ACC_PUBLIC)
```

然后实现接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, close)
{
    int ret;
    zend_long fd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(fd)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket sock(fd);
    ret = sock.close();
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "close error");
        RETURN_FALSE;
    }
    RETURN_LONG(ret);
}
```

然后编译、安装扩展：

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

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        var_dump($connfd);
        $serv->close($connfd);
    }
});

Sco::scheduler();
```

启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后用客户端多次去连接然后直接断开连接：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test #
```

我们发现，连接套接字不再递增了：

```shell
~/codeDir/cppCode/study # php test.php
int(11)
int(11)
int(11)

```

有了这个`close`功能，也可以减少连接泄漏的问题。
