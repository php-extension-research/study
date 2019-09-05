# 协程化服务器（一）

到目前为止，我们已经协程化了我们基本的`Socket`方法。现在，我们需要通过这些方法，协程化我们的服务器。

首先，我们需要去改造接口：

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
```

因为我们使用到了协程化的方法，所以，我们需要在头文件`study_server_coro.h`里面引入头文件`coroutine_socket.h`：

```cpp
#ifndef STUDY_SERVER_CORO_H
#define STUDY_SERVER_CORO_H

#include "php_study.h"
#include "socket.h"
#include "coroutine_socket.h" // 新增的一行
#include "error.h"

#endif	/* STUDY_SERVER_CORO_H */
```

然后，在文件`study_server_coro.cc`头部引用命名空间：

```cpp
#include "study_server_coro.h"

using namespace study::coroutine::Socket; // 新增的一行
```

然后，我们修改接口代码：

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
{
    zval *zhost;
    zend_long zport;
    zval zsock;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(zport)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket sock(AF_INET, SOCK_STREAM, 0);
    sock.bind(ST_SOCK_TCP, Z_STRVAL_P(zhost), zport);
    sock.listen();

    ZVAL_PTR(&zsock, &sock);

    zend_update_property(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("zsock"), &zsock);
    zend_update_property_string(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("host"), Z_STRVAL_P(zhost));
    zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("port"), zport);
}
```

这里，我把非阻塞的`sokcet`函数都换成了`coroutine::Socket`类里面的方法。并且，把创建出来的`sock`对象放进了`zsock`这个`zval`容器里面。然后，我们再设置到`coroutine_server_coro`这个`PHP`对象的`zsock`属性里面。这里，我把属性`sock`改成了名字`zsock`，所以在声明这个属性的地方，我们也改一下。在函数`study_coroutine_server_coro_init`里面：

```cpp
zend_declare_property(study_coroutine_server_coro_ce_ptr, ZEND_STRL("zsock"), NULL, ZEND_ACC_PUBLIC);
```

因为这个属性是一个指针，所以我们给它的默认值是`NULL`。

`OK`，我们来编译一下扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install

----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

编译成功。

我们编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
```

然后执行：

```cpp
~/codeDir/cppCode/study # php test.php 
PHP Warning:  PHP Startup: Unable to load dynamic library 'study.so' (tried: /usr/local/lib/php/extensions/no-debug-non-zts-20180731/study.so (Error relocating /usr/local/lib/php/extensions/no-debug-non-zts-20180731/study.so: _ZN5study9coroutine6SocketD1Ev: symbol not found), /usr/local/lib/php/extensions/no-debug-non-zts-20180731/study.so.so (Error loading shared library /usr/local/lib/php/extensions/no-debug-non-zts-20180731/study.so.so: No such file or directory)) in Unknown on line 0

Fatal error: Uncaught Error: Class 'Study\Coroutine\Server' not found in /root/codeDir/cppCode/study/test.php:3
Stack trace:
#0 {main}
  thrown in /root/codeDir/cppCode/study/test.php on line 3
~/codeDir/cppCode/study # 
```

这里报错，说是

```
_ZN5study9coroutine6SocketD1Ev: symbol not found
```

意思是说，我们的

```cpp
study::coroutine::Socket::~Socket()
```

没有找到。因为我们没有去实现它。

为什么它会去调用我们的析构函数呢？因为我们的`Socket`对象是在栈上面分配的内存，所以，一旦函数

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
```

执行完毕，这个对象就会被销毁，在销毁的时候就会自动的调用这个类的析构函数。

所以，我们来定义一下这个析构函数，在文件`src/coroutine/socket.cc`里面：

```cpp
Socket::~Socket()
{
}
```

因为我们目前不需要去释放任何的资源，所以方法体里面就不写代码了。

再然后，我们显然不希望我们的`Socket`对象在调用完

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
```

之后就被释放掉了，所以我们应该在堆上面分配内存，`__construct`接口修改如下：

```cpp
PHP_METHOD(study_coroutine_server_coro, __construct)
{
    zval *zhost;
    zend_long zport;
    zval zsock;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(zport)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket *sock = new Socket(AF_INET, SOCK_STREAM, 0);

    sock->bind(ST_SOCK_TCP, Z_STRVAL_P(zhost), zport);
    sock->listen();

    ZVAL_PTR(&zsock, sock);

    zend_update_property(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("zsock"), &zsock);
    zend_update_property_string(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("host"), Z_STRVAL_P(zhost));
    zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("port"), zport);
}
```

[下一篇：协程化服务器（二）](./《PHP扩展开发》-协程-协程化服务器（二）.md)

