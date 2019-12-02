# 如何bind和listen

在上篇文章中，我们成功的创建出了一个`php_stream`类型的资源对象。这篇文章，我们来实现一下对`socket`的`bind`和`listen`。测试脚本如下：

```php
<?php

study_event_init();

Sgo(function () {
    $ctx = stream_context_create(['socket' => ['so_reuseaddr' => true, 'backlog' => 128]]);
    $socket = stream_socket_server(
        'tcp://0.0.0.0:6666',
        $errno,
        $errstr,
        STREAM_SERVER_BIND | STREAM_SERVER_LISTEN,
        $ctx
    );
    if (!$socket) {
        echo "$errstr ($errno)" . PHP_EOL;
        exit(1);
    }
    var_dump($socket);
    sleep(10000);
});

study_event_wait();
```

这段代码很简单。我们没有开启`hook`功能，所以用的是`PHP`内核原来的函数。执行脚本：

```shell
[root@7b6ef640478b study]# php test.php
resource(5) of type (stream)

```

此时，另起一个终端，查看端口情况：

```shell
[root@7b6ef640478b test]# netstat -antp | grep 6666
tcp        0      0 0.0.0.0:6666            0.0.0.0:*               LISTEN      37874/php
[root@7b6ef640478b test]#
```

我们发现，测试会用端口占用。也就是说，当我们调用`stream_socket_server`，并且指定了`STREAM_SERVER_BIND`和`STREAM_SERVER_LISTEN`标志，那么就会占用端口。现在，我们开启`hook`功能，测试代码如下：

```php
<?php

study_event_init();

Study\Runtime::enableCoroutine();

Sgo(function () {
    $ctx = stream_context_create(['socket' => ['so_reuseaddr' => true, 'backlog' => 128]]);
    $socket = stream_socket_server(
        'tcp://0.0.0.0:6666',
        $errno,
        $errstr,
        STREAM_SERVER_BIND | STREAM_SERVER_LISTEN,
        $ctx
    );
    if (!$socket) {
        echo "$errstr ($errno)" . PHP_EOL;
        exit(1);
    }
    var_dump($socket);
    sleep(10000);
});

study_event_wait();
```

执行结果：

```shell
[root@7b6ef640478b study]# php test.php
resource(5) of type (stream)

```

此时，另起一个终端，查看端口占用：

```shell
[root@7b6ef640478b test]# netstat -antp | grep 6666
[root@7b6ef640478b test]#
```

发现没有占用端口。说明没有调用操作系统的`bind`和`listen`接口。

分析没有`hook`的内核源码可以知道，`bind`和`listen`的操作是在`socket_ops`的`socket_set_option`函数里面完成的。因为我们的这个函数没有去实现，所以自然就无法`bind`和`listen`端口了。我们这里来实现一下：

```cpp
static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam)
{
    php_study_netstream_data_t *abstract = (php_study_netstream_data_t *) stream->abstract;
    php_stream_xport_param *xparam;

    switch(option)
    {
    case PHP_STREAM_OPTION_XPORT_API:
        xparam = (php_stream_xport_param *)ptrparam;

        switch(xparam->op)
        {
        case STREAM_XPORT_OP_BIND:
            xparam->outputs.returncode = php_study_tcp_sockop_bind(stream, abstract, xparam);
            return PHP_STREAM_OPTION_RETURN_OK;
        case STREAM_XPORT_OP_LISTEN:
            xparam->outputs.returncode = abstract->socket->listen(xparam->inputs.backlog);
            return PHP_STREAM_OPTION_RETURN_OK;
        default:
            /* fall through */
            ;
        }
    }
    return PHP_STREAM_OPTION_RETURN_OK;
}
```

当我们在`PHP`函数`stream_socket_server`中设置了`STREAM_SERVER_BIND`或者`STREAM_SERVER_LISTEN`，实际上`PHP`底层就会调用到我们的这个`socket_set_option`函数。我们来分析一下这段代码。核心代码就是这个`switch`语句了：

```cpp
switch(xparam->op)
{
case STREAM_XPORT_OP_BIND:
    xparam->outputs.returncode = php_study_tcp_sockop_bind(stream, abstract, xparam);
    return PHP_STREAM_OPTION_RETURN_OK;
case STREAM_XPORT_OP_LISTEN:
    xparam->outputs.returncode = abstract->socket->listen(xparam->inputs.backlog);
    return PHP_STREAM_OPTION_RETURN_OK;
default:
    /* fall through */
    ;
}
```

这个`xparam->op`此时对应着我们在`stream_socket_server`中设置的`$flags`，也就是`STREAM_SERVER_BIND | STREAM_SERVER_LISTEN`。所以，这里两个`case`都会命中。（注意，虽然`switch`这里只会执行其中的一个`case`，但是，内核会分别两次调用我们的`socket_set_option`函数）

这里我们需要定义一下`STREAM_XPORT_OP_BIND`和`STREAM_XPORT_OP_LISTEN`：

```cpp
enum
{
    STREAM_XPORT_OP_BIND,
    STREAM_XPORT_OP_CONNECT,
    STREAM_XPORT_OP_LISTEN,
    STREAM_XPORT_OP_ACCEPT,
    STREAM_XPORT_OP_CONNECT_ASYNC,
    STREAM_XPORT_OP_GET_NAME,
    STREAM_XPORT_OP_GET_PEER_NAME,
    STREAM_XPORT_OP_RECV,
    STREAM_XPORT_OP_SEND,
    STREAM_XPORT_OP_SHUTDOWN,
};
```

这个要和`PHP`内核里面的顺序一致。然后我们需要实现`php_study_tcp_sockop_bind`这个函数：

```cpp
static int php_study_tcp_sockop_bind(php_stream *stream, php_study_netstream_data_t *abstract, php_stream_xport_param *xparam)
{
    char *host = NULL;
    int portno;

    host = parse_ip_address(xparam, &portno);

    if (host == NULL)
    {
        return -1;
    }

    int ret = abstract->socket->bind(ST_SOCK_TCP, host, portno);

    if (host)
    {
        efree(host);
    }
    return ret;
}
```

代码很简单，就是去调用我们封装好的`bind`函数。但是，这里有一点需要注意的是，我们需要从`stream_socket_server`函数的第一个参数里面解析出`host`和`port`。这里就是通过`parse_ip_address`来实现的。具体细节我们无需过多的关注，只需要知道这个函数是从`PHP`内核复制出来的即可：

```cpp
/**
 * copy from php src file: xp_socket.c
 */
static inline char *parse_ip_address_ex(const char *str, size_t str_len, int *portno, int get_err, zend_string **err)
{
    char *colon;
    char *host = NULL;

#ifdef HAVE_IPV6
    char *p;

    if (*(str) == '[' && str_len > 1)
    {
        /* IPV6 notation to specify raw address with port (i.e. [fe80::1]:80) */
        p = (char *)memchr(str + 1, ']', str_len - 2);
        if (!p || *(p + 1) != ':')
        {
            if (get_err)
            {
                *err = strpprintf(0, "Failed to parse IPv6 address \"%s\"", str);
            }
            return NULL;
        }
        *portno = atoi(p + 2);
        return estrndup(str + 1, p - str - 1);
    }
#endif
    if (str_len)
    {
        colon = (char *)memchr(str, ':', str_len - 1);
    }
    else
    {
        colon = NULL;
    }
    if (colon)
    {
        *portno = atoi(colon + 1);
        host = estrndup(str, colon - str);
    }
    else
    {
        if (get_err)
        {
            *err = strpprintf(0, "Failed to parse address \"%s\"", str);
        }
        return NULL;
    }

    return host;
}

/**
 * copy from php src file: xp_socket.c
 */
static inline char *parse_ip_address(php_stream_xport_param *xparam, int *portno)
{
    return parse_ip_address_ex(xparam->inputs.name, xparam->inputs.namelen, portno, xparam->want_errortext, &xparam->outputs.error_text);
}
```

`OK`，我们现在实现完了`socket_set_option`这个函数。我们重新编译、安装扩展：

```shell
[root@7b6ef640478b study]# make clean && make install
----------------------------------------------------------------------
Installing shared extensions:     /usr/lib/php/extensions/debug-non-zts-20180731/
Installing header files:          /usr/include/php/
[root@7b6ef640478b study]#
```

编写测试脚本：

```shell
<?php

study_event_init();

Study\Runtime::enableCoroutine();

Sgo(function () {
    $ctx = stream_context_create(['socket' => ['so_reuseaddr' => true, 'backlog' => 128]]);
    $socket = stream_socket_server(
        'tcp://0.0.0.0:6666',
        $errno,
        $errstr,
        STREAM_SERVER_BIND | STREAM_SERVER_LISTEN,
        $ctx
    );
    if (!$socket) {
        echo "$errstr ($errno)" . PHP_EOL;
        exit(1);
    }
    var_dump($socket);
    sleep(10000);
});

study_event_wait();
```

执行脚本：

```shell
[root@7b6ef640478b study]# php test.php
resource(5) of type (stream)

```

然后查看端口占用情况：

```shell
[root@7b6ef640478b ~]# netstat -antp | grep 6666
tcp        0      0 0.0.0.0:6666            0.0.0.0:*               LISTEN      6822/php
[root@7b6ef640478b ~]#
```

符合预期。
