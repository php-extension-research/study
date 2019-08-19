# server创建（二）

上一篇文章，我们成功的注册了`Server`类。现在，我们需要去实现这个类的第一个方法`__construct`。

首先，我们现在定义一下这个构造函数的参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_server_coro_construct, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()
```

然后，我们定义构造函数：

```cpp
PHP_METHOD(study_coroutine_util, __construct)
{
    int sock;
    zval *zhost;
    zend_long zport;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(zport)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
}
```

这里，解析除了我们传递的`host`以及`port`。然后，我们需要去实现创建`socket`的代码。

我们先创建文件`include/socket.h`，内容如下：

```cpp
#ifndef SOCKET_H
#define SOCKET_H

#include "study.h"

int stSocket_create(int type);
int stSocket_bind(int sock, int type, char *host, int port);

#endif	/* SOCKET_H */
```

然后再创建文件`src/socket.cc`，内容如下：

```cpp
#include "socket.h"
```

我们先来实现一下`stSocket_create`函数，这个函数是用来创建一个服务端用的套接字，代码如下：

```cpp
int stSocket_create(int type)
{
    int _domain;
    int _type;

    if (type == ST_SOCK_TCP)
    {
        _domain = AF_INET;
        _type = SOCK_STREAM;
    }
    else if (type == ST_SOCK_UDP)
    {
        _domain = AF_INET;
        _type = SOCK_DGRAM;
    }
    else
    {
        return -1;
    }

    return socket(_domain, _type, 0);
}
```

这段代码很简单，就是根据传递的`type`类型来创建套接字，然后返回这个套接字，简单的对`socket()`函数封装了一下。

接下来，我们去实现一下`stSocket_bind`：

```cpp
int stSocket_bind(int sock, int type, char *host, int port)
{
    int ret;
    struct sockaddr_in servaddr;

    if (type == ST_SOCK_TCP)
    {
        bzero(&servaddr, sizeof(servaddr));
        inet_aton(host, &(servaddr.sin_addr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        ret = bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (ret < 0)
        {
            return -1; 
        }
    }
    else
    {
        return -1;
    }

    return ret;
}
```

这段代码也很简单，对`bind()`函数进行了封装。`OK`，现在我们可以在构造函数里面去调用这两个函数了。

我们先在`study_server_coro.h`里面引入`socket.h`：

```cpp
#include "socket.h"
```

然后我们继续实现构造函数的代码：

```cpp
sock = stSocket_create(ST_SOCK_TCP);
stSocket_bind(sock, ST_SOCK_TCP, Z_STRVAL_P(zhost), zport);
listen(sockfd, 512);

zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("sock"), sock);
zend_update_property_string(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("host"), Z_STRVAL_P(zhost));
zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("port"), zport);
```

我们创建完了套接字之后，然后把`sock`、`host`、`port`保存下来作为`server`对象的属性。

然后，我们注册这个构造函数：

```cpp
static const zend_function_entry study_coroutine_server_coro_methods[] =
{
	PHP_ME(study_coroutine_server_coro, __construct, arginfo_study_coroutine_server_coro_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_FE_END
};
```

最后，我们修改`study_coroutine_server_coro_init`模块初始化的内容，声明类的属性以及方法：

```cpp
void study_coroutine_server_coro_init()
{
    INIT_NS_CLASS_ENTRY(study_coroutine_server_coro_ce, "Study", "Coroutine\\Server", study_coroutine_server_coro_methods);
    study_coroutine_server_coro_ce_ptr = zend_register_internal_class(&study_coroutine_server_coro_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property_long(study_coroutine_server_coro_ce_ptr, ZEND_STRL("sock"), -1, ZEND_ACC_PUBLIC);
	zend_declare_property_string(study_coroutine_server_coro_ce_ptr, ZEND_STRL("host"), "", ZEND_ACC_PUBLIC);
	zend_declare_property_long(study_coroutine_server_coro_ce_ptr, ZEND_STRL("port"), -1, ZEND_ACC_PUBLIC);
}
```

因为我们增加了需要被编译的`src/socket.cc`文件，所以我们需要修改`config.m4`文件：

```shell
study_source_file="\
    study.cc \
    study_coroutine.cc \
    study_coroutine_util.cc \
    src/coroutine/coroutine.cc \
    src/coroutine/context.cc \
    ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
    ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE} \
    study_server_coro.cc \
    src/socket.cc
"
```

然后执行：

```shell
~/codeDir/cppCode/study # phpize --clean && phpize && ./configure
```

接着编译、安装扩展：

```shell
~/codeDir/cppCode/study # make && make install
```

编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
var_dump($serv);
```

执行结果如下：

```shell
object(Study\Coroutine\Server)#1 (3) {
  ["sock"]=>
  int(3)
  ["host"]=>
  string(9) "127.0.0.1"
  ["port"]=>
  int(8080)
}
~/codeDir/cppCode/study # 
```

符合预期。