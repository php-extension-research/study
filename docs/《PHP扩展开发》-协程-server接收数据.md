# server接收数据

这一篇文章我们来实现一下服务器的`recv`。接口如下：

```php
Study\Coroutine\Server->recv(int fd, int length = 65535): string | false;
```

首先，我们需要定义接口的参数，在文件`study_server_coro.cc`里面：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_server_coro_recv, 0, 0, 2)
    ZEND_ARG_INFO(0, fd)
    ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()
```

然后实现`recv`接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, recv)
{
    int ret;
    zend_long fd;
    zend_long length = 65536;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(fd)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    zend_string *buf = zend_string_alloc(length, 0);

    ret = stSocket_recv(fd, ZSTR_VAL(buf), length, 0);
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "recv error");
        RETURN_FALSE;
    }
    ZSTR_VAL(buf)[ret] = '\0';
    RETURN_STR(buf);
}
```

首先，我们要明确一点，我们的`length`代表的是字符串的长度，不包括字符串结束符号`\0`。并且，`zend_string`是需要字符串结束符号。那么，为什么我们`zend_string_alloc`只传递了`length`而不是`length + 1`呢？因为`zend_string_alloc`底层帮我们做好了封装，我们只需要传递字符串的长度即可，`zend_string_alloc`里面会帮我们`+1`。

`stSocket_recv`用来接收客户端发来的数据。我们先在`include/socket.h`里面声明这个函数：

```cpp
ssize_t stSocket_recv(int sock, void *buf, size_t len, int flag)
```

然后在文件`src/socket.cc`里面进行实现：

```cpp
ssize_t stSocket_recv(int sock, void *buf, size_t len, int flag)
{
    ssize_t ret;

    ret = recv(sock, buf, len, flag);
    if (ret < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    return ret;
}
```

最后，我们需要在`zend_string`的数据的后面加上`\0`。

`RETURN_STR(buf);`设置返回值为我们接收到的数据。

然后我们注册这个方法：

```cpp
static const zend_function_entry study_coroutine_server_coro_methods[] =
{
    // 省略其他注册的方法
    PHP_ME(study_coroutine_server_coro, recv, arginfo_study_coroutine_server_coro_recv, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

现在重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
$sock = $serv->accept();
$buf = $serv->recv($sock);
var_dump($buf);
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 

```

然后另开一个终端发送数据：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello world

```

服务器这端将会打印出如下内容：

```shell
~/codeDir/cppCode/study # php test.php 
string(65536) "hello world
"
```

`OK`，符合预期。

