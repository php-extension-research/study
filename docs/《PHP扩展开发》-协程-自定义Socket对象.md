# 自定义Socket对象

因为这这个过程和自定义`Channel`存储对象非常的类似，所以这篇文章不会很啰嗦。

首先，增加文件`study_coroutine_socket.h`里面的内容如下：

```cpp
#ifndef STUDY_COROUTINE_SOCKET_H
#define STUDY_COROUTINE_SOCKET_H

#include "php_study.h"
#include "socket.h"
#include "error.h"
#include "coroutine_socket.h"

#endif  /* STUDY_COROUTINE_SOCKET_H */
```

然后创建文件`study_coroutine_socket.cc`。然后记得修改`config.m4`文件，并且重新编译出一份`Makefile`。

接着，在文件`study_coroutine_socket.cc`里面引入头文件：

```cpp
#include "study_coroutine_socket.h"

using study::coroutine::Socket;
```

然后定义自定义的`Socket`存储对象：

```cpp
typedef struct
{
    Socket *socket;
    zend_object std;
} coro_socket;
```

然后定义一些接口的参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_construct, 0, 0, 2)
    ZEND_ARG_INFO(0, domain)
    ZEND_ARG_INFO(0, type)
    ZEND_ARG_INFO(0, protocol)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_bind, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_listen, 0, 0, 0)
    ZEND_ARG_INFO(0, backlog)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_recv, 0, 0, 0)
    ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_send, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()
```

然后定义指向`Study\Coroutine\Socket`类的指针：

```cpp
/**
 * Define zend class entry
 */
zend_class_entry study_coro_socket_ce;
zend_class_entry *study_coro_socket_ce_ptr;
```

然后定义自定义对象的`handler`：

```cpp
static zend_object_handlers study_coro_socket_handlers;
```

接着，实现一些基本的函数：

```cpp
static coro_socket* study_coro_socket_fetch_object(zend_object *obj)
{
    return (coro_socket *) ((char *) obj - study_coro_socket_handlers.offset);
}

static zend_object* study_coro_socket_create_object(zend_class_entry *ce)
{
    coro_socket *sock = (coro_socket *) ecalloc(1, sizeof(coro_socket) + zend_object_properties_size(ce));
    zend_object_std_init(&sock->std, ce);
    object_properties_init(&sock->std, ce);
    sock->std.handlers = &study_coro_socket_handlers;
    return &sock->std;
}

static void study_coro_socket_free_object(zend_object *object)
{
    coro_socket *sock = (coro_socket *) study_coro_socket_fetch_object(object);
    if (sock->socket && sock->socket != ST_BAD_SOCKET)
    {
        sock->socket->close();
        delete sock->socket;
    }
    zend_object_std_dtor(&sock->std);
}
```

其中，`fetch_object`和`create_object`不多啰嗦，和`Channel`的基本一样。这里看看`free_object`。

主要是`if`语句那一块：

```cpp
if (sock->socket && sock->socket != ST_BAD_SOCKET)
```

这个语句主要是判断这个`Socket`是否已经被释放过了。如果之前已经释放过了，那么`sock->socket`会等于`ST_BAD_SOCKET`，此时，就无须再次释放一次了，可以跳过`if`里面的代码。

我们需要定义一下这个宏`ST_BAD_SOCKET`：

```cpp
#define ST_BAD_SOCKET ((Socket *)-1)
```

那么，什么时候`Socket`会被释放呢？当在`PHP`脚本里面调用了`close`接口。（这个接口我们后面会去实现它）

接着，我们去实现这些接口，首先是构造函数：

```cpp
static PHP_METHOD(study_coro_socket, __construct)
{
    zend_long domain;
    zend_long type;
    zend_long protocol;
    coro_socket *socket_t;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_LONG(domain)
        Z_PARAM_LONG(type)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(protocol)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket_t->socket = new Socket(domain, type, protocol);

    zend_update_property_long(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("fd"), socket_t->socket->get_fd());
}
```

然后，实现`bind`接口：

```cpp
static PHP_METHOD(study_coro_socket, bind)
{
    zend_long port;
    Socket *socket;
    coro_socket *socket_t;
    zval *zhost;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket = socket_t->socket;

    if (socket->bind(ST_SOCK_TCP, Z_STRVAL_P(zhost), port) < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

然后实现`listen`接口：

```cpp
static PHP_METHOD(study_coro_socket, listen)
{
    zend_long backlog = 512;
    Socket *socket;
    coro_socket *socket_t;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(backlog)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket = socket_t->socket;

    if (socket->listen(backlog) < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

这里，我们需要修改底层`Socket`类的`listen`方法，我们增加了`backlog`参数。首先，在文件`include/socket.h`里面：

```cpp
int stSocket_listen(int sock, int backlog);
```

然后是文件`src/socket.cc`里面：

```cpp
int stSocket_listen(int sock, int backlog)
{
    int ret;

    ret = listen(sock, backlog);
    if (ret < 0)
    {
        stError("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    return ret;
}
```

然后是文件`include/coroutine_socket.h`里面：

```cpp
int listen(int backlog);
```

然后是文件`src/coroutine/socket.cc`里面：

```cpp
int Socket::listen(int backlog)
{
    return stSocket_listen(sockfd, backlog);
}
```

此时，我们先去掉对`study_server_coro.cc`文件的编译，因为我们的修改会让`Study\Coroutine\Server`的编译失败，我们等`Socket`自定义对象实现完了之后，我们再处理`Server`部分。所以，在`config.m4`文件里面，删除对`study_server_coro.cc`的编译，并且，重新生成一份`Makefile`。并且在模块初始化里面注释掉对`study_coroutine_server_coro_init`的调用，如下所示：

```cpp
PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    // study_coroutine_server_coro_init();
    study_coro_channel_init();
    return SUCCESS;
}
```

接着，我们去实现`accept`接口：

```cpp
static PHP_METHOD(study_coro_socket, accept)
{
    coro_socket *server_socket_t;
    coro_socket *conn_socket_t;
    Socket *server_socket;
    Socket *conn_socket;

    server_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    server_socket = server_socket_t->socket;
    conn_socket = server_socket->accept();
    if (!conn_socket)
    {
        RETURN_NULL();
    }
    zend_object *conn = study_coro_socket_create_object(study_coro_socket_ce_ptr);
    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(conn);
    conn_socket_t->socket = conn_socket;
    ZVAL_OBJ(return_value, &(conn_socket_t->std));
    zend_update_property_long(study_coro_socket_ce_ptr, return_value, ZEND_STRL("fd"), conn_socket_t->socket->get_fd());
}
```

这里，我们对底层的`accept`方法进行了修改。这里不是直接返回文件描述符，而是一个封装好了的`Socket`对象。

首先是文件`coroutine_socket.h`：

```cpp
Socket* accept();
```

然后是文件`src/coroutine/socket.cc`：

```cpp
Socket* Socket::accept()
{
    int connfd;

    do
    {
        connfd = stSocket_accept(sockfd);
    } while (connfd < 0 && errno == EAGAIN && wait_event(ST_EVENT_READ));

    return (new Socket(connfd));
}
```

接着，实现`recv`接口：

```cpp
static PHP_METHOD(study_coro_socket, recv)
{
    ssize_t ret;
    zend_long length = 65536;
    coro_socket *conn_socket_t;
    Socket *conn_socket;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket::init_read_buffer();

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    conn_socket = conn_socket_t->socket;
    ret = conn_socket->recv(Socket::read_buffer, Socket::read_buffer_len);
    if (ret == 0)
    {
        zend_update_property_long(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("errCode"), ST_ERROR_SESSION_CLOSED_BY_CLIENT);
        zend_update_property_string(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("errMsg"), st_strerror(ST_ERROR_SESSION_CLOSED_BY_CLIENT));
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

接着，实现`send`接口：

```cpp
static PHP_METHOD(study_coro_socket, send)
{
    ssize_t ret;
    char *data;
    size_t length;
    coro_socket *conn_socket_t;
    Socket *conn_socket;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    conn_socket = conn_socket_t->socket;
    ret = conn_socket->send(data, length);
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "send error");
        RETURN_FALSE;
    }
    RETURN_LONG(ret);
}
```

接着，实现`close`接口：

```cpp
static PHP_METHOD(study_coro_socket, close)
{
    int ret;
    coro_socket *conn_socket_t;

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    ret = conn_socket_t->socket->close();
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "close error");
        RETURN_FALSE;
    }
    delete conn_socket_t->socket;
    conn_socket_t->socket = ST_BAD_SOCKET;

    RETURN_TRUE;
}
```

接着，注册接口：

```cpp
static const zend_function_entry study_coro_socket_methods[] =
{
    PHP_ME(study_coro_socket, __construct, arginfo_study_coro_socket_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(study_coro_socket, bind, arginfo_study_coro_socket_bind, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, listen, arginfo_study_coro_socket_listen, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, accept, arginfo_study_coro_socket_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, recv, arginfo_study_coro_socket_recv, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, send, arginfo_study_coro_socket_send, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, close, arginfo_study_coro_socket_void, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

然后，进行模块初始化，注册类，注册一些常量，修改对象的`handler`：

```cpp
void study_coro_socket_init(int module_number)
{
    INIT_NS_CLASS_ENTRY(study_coro_socket_ce, "Study", "Coroutine\\Socket", study_coro_socket_methods);
    memcpy(&study_coro_socket_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    study_coro_socket_ce_ptr = zend_register_internal_class(&study_coro_socket_ce TSRMLS_CC); // Registered in the Zend Engine
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_socket, study_coro_socket_create_object, study_coro_socket_free_object, coro_socket, std);

    zend_declare_property_long(study_coro_socket_ce_ptr, ZEND_STRL("fd"), -1, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_socket_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coro_socket_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);

    if (!zend_hash_str_find_ptr(&module_registry, ZEND_STRL("sockets")))
    {
        REGISTER_LONG_CONSTANT("AF_INET", AF_INET, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("SOCK_STREAM", SOCK_STREAM, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("IPPROTO_IP", IPPROTO_IP, CONST_CS | CONST_PERSISTENT);
    }
}
```

然后，在文件`study.cc`里面的模块初始化里面调用这个`study_coro_socket_init`。首先，得在文件`php_study.h`里面进行声明：

```cpp
void study_coro_socket_init(int module_number);
```

然后在`study.cc`里面进行调用：

```cpp
PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    study_coroutine_server_coro_init();
    study_coro_channel_init();
    study_coro_socket_init(module_number); // 新增的一行
    return SUCCESS;
}
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
```

然后编写测试脚本：

```php
<?php

$socket = new Study\Coroutine\Socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
var_dump($socket);
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php
/root/codeDir/cppCode/study/test.php:5:
class Study\Coroutine\Socket#1 (3) {
  public $fd =>
  int(3)
  public $errCode =>
  int(0)
  public $errMsg =>
  string(0) ""
}
~/codeDir/cppCode/study #
```

接着，我们用`Socket`类编写一个高性能的服务器：

```php

<?php

study_event_init();

Sgo(function ()
{
    $socket = new Study\Coroutine\Socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    $socket->bind("127.0.0.1", 80);
    $socket->listen();
    while (true) {
        $conn = $socket->accept();
        Sgo(function () use($conn)
        {
            $data = $conn->recv();
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $conn->send($responseStr);
            $conn->close();
        });
    }
});

study_event_wait();
```

然后启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
Concurrency Level:      100
Time taken for tests:   0.758 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    13187.62 [#/sec] (mean)
Time per request:       7.583 [ms] (mean)
Time per request:       0.076 [ms] (mean, across all concurrent requests)
Transfer rate:          1236.34 [Kbytes/sec] received
```

符合预期。

[下一篇：修复一些bug（十一）](./《PHP扩展开发》-协程-修复一些bug（十一）.md)
