# 重构Server

这篇文章，我们来重构一下我们的`Study\Coroutine\Server`。

我们先删除文件`study_server_coro.cc`以及`study_server_coro.h`。然后创建文件`study_coroutine_server.cc`以及`study_coroutine_server.h`。

然后修改`config.m4`文件，添加`study_coroutine_server.cc`为需要编译的源文件。然后编译出一份新的`Makefile`。

其中，`study_coroutine_server.h`里面的内容如下：

```cpp
#ifndef STUDY_COROUTINE_SERVER_H
#define STUDY_COROUTINE_SERVER_H

#include "php_study.h"
#include "study_coroutine.h"
#include "socket.h"
#include "coroutine_socket.h"
#include "error.h"

namespace study { namespace phpcoroutine {
class Server
{
private:
    study::coroutine::Socket *socket = nullptr;
    php_study_fci_fcc *handler = nullptr;
    bool running = false;

public:
    Server(char *host, int port);
    ~Server();
    bool start();
    bool shutdown();
    void set_handler(php_study_fci_fcc *_handler);
    php_study_fci_fcc* get_handler();
};
}
}

#endif /* STUDY_COROUTINE_SERVER_H */
```

我们定义了一个新的类`study::phpcoroutine::Server`，这个类就是给扩展接口用的。

其中，

```cpp
study::coroutine::Socket *socket
```

是这个服务器所使用过的`socket`。

```cpp
php_study_fci_fcc *handler = nullptr;
```

用来保存接收到客户端连接之后，会回调的函数。

```cpp
bool running = false;
```

用来判断这个服务器是否正在运行中。

```cpp
bool start();
bool shutdown();
```

分别用来启动服务器以及关闭服务器。

```cpp
void set_handler(php_study_fci_fcc *_handler);
php_study_fci_fcc* get_handler();
```

分别用来设置回调函数以及获得回调函数。

接着，我们去实现这些方法，在文件`study_coroutine_server.cc`里面。

我们先引入头部内容：

```cpp
#include "study_coroutine_server.h"
#include "study_coroutine_socket.h"
#include "log.h"

using study::phpcoroutine::Server;
using study::PHPCoroutine;
using study::coroutine::Socket;
```

然后实现构造函数：

```cpp
Server::Server(char *host, int port)
{
    socket = new Socket(AF_INET, SOCK_STREAM, 0);
    if (socket->bind(ST_SOCK_TCP, host, port) < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return;
    }
    if (socket->listen(512) < 0)
    {
        return;
    }
}
```

代码很简单，就是去创建一个服务端使用的套接字，然后绑定对应的`ip`以及端口，然后设置`backlog`队列的大小。

然后是析构函数：

```cpp
Server::~Server()
{
}
```

暂时没有需要被释放的资源。

然后是启动服务器的代码：

```cpp
bool Server::start()
{
    zval zsocket;
    running = true;

    while (running)
    {
        Socket* conn = socket->accept();
        if (!conn)
        {
            return false;
        }

        php_study_init_socket_object(&zsocket, conn);
        PHPCoroutine::create(&(handler->fcc), 1, &zsocket);
        zval_dtor(&zsocket);
    }
    return true;
}
```

代码也比较简单。首先，把`running`设置为`true`，代表服务器已经启动了。然后，执行一个无限循环，然后在循环里面去获取客户端的连接。获取到连接之后，就创建一个协程，去执行用户设置的回调函数。回调函数的参数只有一个，就是对应的连接`socket`，它是`Study\Coroutine\Socket`类型。

其中，`php_study_init_socket_object`的作用是去初始化一个自定义的`PHP`对象，并且让`zsocket`这个容器指向自定义对象里面的`std`对象。我们在文件`study_coroutine_socket.cc`里面去实现这个函数：

```cpp
void php_study_init_socket_object(zval *zsocket, Socket *socket)
{
    zend_object *object = study_coro_socket_create_object(study_coro_socket_ce_ptr);

    coro_socket *socket_t = (coro_socket *) study_coro_socket_fetch_object(object);
    socket_t->socket = socket;
    ZVAL_OBJ(zsocket, object);
}
```

然后，在文件`study_coroutine_socket.h`里面去声明这个函数：

```cpp
void php_study_init_socket_object(zval *zsocket, study::coroutine::Socket *socket);
```

这样，在文件`study_coroutine_server.cc`里面就可以调用`php_study_init_socket_object`了。

接着，我们去实现关闭服务器的方法：

```cpp
bool Server::shutdown()
{
    running = false;
    return true;
}
```

然后，实现设置回调函数的方法：

```cpp
void Server::set_handler(php_study_fci_fcc *_handler)
{
    handler = _handler;
}
```

然后，实现获取回调函数的代码：

```cpp
php_study_fci_fcc* Server::get_handler()
{
    return handler;
}
```

这样，我们就实现完`study::phpcoroutine::Server`这个类了。我们接下来去实现扩展接口部分。

首先，定义`PHP`类指针以及对象的`handler`：

```cpp
/**
 * Define zend class entry
 */
zend_class_entry study_coro_server_ce;
zend_class_entry *study_coro_server_ce_ptr;

static zend_object_handlers study_coro_server_handlers;
```

然后定义自定义`PHP`对象：

```cpp
typedef struct
{
    Server *serv;
    zend_object std;
} coro_serv;
```

然后，实现一下创建自定义对象以及获取自定义对象的函数：

```cpp
static coro_serv* study_coro_server_fetch_object(zend_object *obj)
{
    return (coro_serv *)((char *)obj - study_coro_server_handlers.offset);
}

static zend_object* study_coro_server_create_object(zend_class_entry *ce)
{
    coro_serv *serv_t = (coro_serv *)ecalloc(1, sizeof(coro_serv) + zend_object_properties_size(ce));
    zend_object_std_init(&serv_t->std, ce);
    object_properties_init(&serv_t->std, ce);
    serv_t->std.handlers = &study_coro_server_handlers;
    return &serv_t->std;
}
```

然后实现释放自定义对象的函数：

```cpp
static void study_coro_server_free_object(zend_object *object)
{
    coro_serv *serv_t = (coro_serv *) study_coro_server_fetch_object(object);
    if (serv_t->serv)
    {
        php_study_fci_fcc *handler = serv_t->serv->get_handler();
        if (handler)
        {
            efree(handler);
            serv_t->serv->set_handler(nullptr);
        }
        delete serv_t->serv;
    }
    zend_object_std_dtor(&serv_t->std);
}
```

这里，我们需要记得去释放`handler`，避免内存泄漏。

接着，我们定义接口的参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_construct, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_ser_handler, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, func, 0)
ZEND_END_ARG_INFO()
```

然后实现构造函数：

```cpp
PHP_METHOD(study_coro_server, __construct)
{
    zend_long port;
    coro_serv *server_t;
    zval *zhost;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    server_t->serv = new Server(Z_STRVAL_P(zhost), port);

    zend_update_property_string(study_coro_server_ce_ptr, getThis(), ZEND_STRL("host"), Z_STRVAL_P(zhost));
    zend_update_property_long(study_coro_server_ce_ptr, getThis(), ZEND_STRL("port"), port);
}
```

代码和之前的类似，就不多啰嗦了。

然后实现启动服务器的代码：

```cpp
PHP_METHOD(study_coro_server, start)
{
    coro_serv *server_t;

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    if (server_t->serv->start() == false)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

代码也很简单，就是直接去调用我们的`start`方法。

然后实现关闭服务器的代码：

```cpp
PHP_METHOD(study_coro_server, shutdown)
{
    coro_serv *server_t;

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    if (server_t->serv->shutdown() == false)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

然后去实现设置回调函数的这个接口：

```cpp
PHP_METHOD(study_coro_server, set_handler)
{
    coro_serv *server_t;
    php_study_fci_fcc *handle_fci_fcc;

    handle_fci_fcc = (php_study_fci_fcc *)emalloc(sizeof(php_study_fci_fcc));

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(handle_fci_fcc->fci, handle_fci_fcc->fcc)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    server_t->serv->set_handler(handle_fci_fcc);
}
```

这里，我们不需要从`PHP`脚本那边传递这个函调函数的参数进来，因为，这个回调函数的默认参数就是`Study\Coroutine\Socket`。我们在方法`study::phpcoroutine::Server::start`里面已经做了这个处理。

接着，我们需要注册这些接口：

```cpp
static const zend_function_entry study_coroutine_server_coro_methods[] =
{
    PHP_ME(study_coro_server, __construct, arginfo_study_coro_server_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_ME(study_coro_server, start, arginfo_study_coro_server_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_server, shutdown, arginfo_study_coro_server_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_server, set_handler, arginfo_study_coro_server_ser_handler, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

然后，实现模块初始化：

```cpp
void study_coro_server_init(int module_number)
{
    INIT_NS_CLASS_ENTRY(study_coro_server_ce, "Study", "Coroutine\\Server", study_coroutine_server_coro_methods);

    memcpy(&study_coro_server_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    study_coro_server_ce_ptr = zend_register_internal_class(&study_coro_server_ce TSRMLS_CC); // Registered in the Zend Engine
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_server, study_coro_server_create_object, study_coro_server_free_object, coro_serv, std);

    zend_declare_property_string(study_coro_server_ce_ptr, ZEND_STRL("host"), "", ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_server_ce_ptr, ZEND_STRL("port"), -1, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_server_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coro_server_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);
}
```

然后，我们需要在文件`study.cc`里面的`PHP_MINIT_FUNCTION(study)`去调用这个模块初始化的代码：

```cpp
PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    study_coro_server_init(module_number); // 修改的地方
    study_coro_channel_init();
    study_coro_socket_init(module_number);
    return SUCCESS;
}
```

然后，我们需要在文件`php_study.h`里面去声明这个函数：

```cpp
void study_coroutine_util_init();
void study_coro_server_init(int module_number); // 修改的一行
void study_coro_channel_init();
void study_coro_socket_init(int module_number);
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

接着，编写测试脚本：

```php
<?php

study_event_init();

Sgo(function ()
{
    $server = new Study\Coroutine\Server("127.0.0.1", 80);
    $server->set_handler(function (Study\Coroutine\Socket $conn) use($server) {
        $data = $conn->recv();
        $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
        $conn->send($responseStr);
        $conn->close();
        // Sco::sleep(0.01);
    });
    $server->start();
});

study_event_wait();
```

然后启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
~/codeDir # ab -c 100 -n 1000000 127.0.0.1/
Concurrency Level:      100
Time taken for tests:   85.210 seconds
Complete requests:      1000000
Failed requests:        0
Total transferred:      96000000 bytes
HTML transferred:       13000000 bytes
Requests per second:    11735.71 [#/sec] (mean)
Time per request:       8.521 [ms] (mean)
Time per request:       0.085 [ms] (mean, across all concurrent requests)
Transfer rate:          1100.22 [Kbytes/sec] received
```

可以看出，在百万级别的请求下，服务器也是可以非常稳定的。

[下一篇：hook原来的sleep](./《PHP扩展开发》-协程-hook原来的sleep.md)
