# 替换php_stream_generic_socket_factory

这篇文章，我们开始`hook`基于`php_stream`的`tcp socket`。

首先，我们修改`PHP_METHOD(study_runtime, enableCoroutine)`这个接口的代码：

```cpp
static PHP_METHOD(study_runtime, enableCoroutine)
{
    hook_func(ZEND_STRL("sleep"), zim_study_coroutine_util_sleep);
    php_stream_xport_register("tcp", socket_create); // 新增的代码
}
```

可以看出来，我们打算用`socket_create`这个函数替换掉原来的`php_stream_generic_socket_factory`。现在，我们来实现一下这个函数。这个函数的函数签名需要和`php_stream_generic_socket_factory`一致才行：

```cpp
php_stream *socket_create(const char *proto, size_t protolen,
    const char *resourcename, size_t resourcenamelen,
    const char *persistent_id, int options, int flags,
    struct timeval *timeout,
    php_stream_context *context STREAMS_DC
)
{

}
```

我们始终要明确的一个目标就是，这个函数是用来创建`php_stream`的。我们只要奔着这个目标走下去，就可以替换掉`php_stream_generic_socket_factory`这个函数。

`OK`，我们往里面实现代码：

```cpp
    php_stream *stream;
    php_study_netstream_data_t *abstract;
    Socket *sock;

    sock = new Socket(AF_INET, SOCK_STREAM, 0);
    abstract = (php_study_netstream_data_t*) ecalloc(1, sizeof(*abstract));
    abstract->socket = sock;
    abstract->stream.socket = sock->get_fd();

    if (timeout)
    {
        abstract->stream.timeout = *timeout;
    }
    else
    {
        abstract->stream.timeout.tv_sec = -1;
    }

    persistent_id = nullptr;
    stream = php_stream_alloc_rel(NULL, abstract, persistent_id, "r+");
    if (stream == NULL)
    {
        delete sock;
    }
    return stream;
```

我们一点一点来分析一下这个代码。

其中，

```cpp
php_study_netstream_data_t *abstract;
```

这个结构体是我们自己定义的：

```cpp
struct php_study_netstream_data_t
{
    php_netstream_data_t stream;
    Socket *socket;
};
```

如果大家还记得上一章的内容的话，我们会发现，`PHP`内核它只用了`php_netstream_data_t`这个结构体，没有像我们一样用一个`Socket`。这么做是为了方便我们去获取`Socket`结构。（后面我们会讲解为什么可以这么做）

因为我们用到了`php_netstream_data_t`，所以我们需要引入这个结构体的头文件。我们在文件`php_study.h`里面引入头文件`php_network.h`：

```cpp
#include "php.h"
#include "php_ini.h"
#include "php_network.h" // 新增的一行
```

因为我们用到了`study::coroutine::Socket`，所以我们需要引入这个头文件。在文件`study_runtime.cc`里面引入即可：

```cpp
#include "study_runtime.h"
#include "study_coroutine_socket.h" // 新增的一行

using study::coroutine::Socket; // 新增的一行
```

我们继续分析后面的代码：

```cpp
sock = new Socket(AF_INET, SOCK_STREAM, 0);
abstract = (php_study_netstream_data_t*) ecalloc(1, sizeof(*abstract));
abstract->socket = sock;
abstract->stream.socket = sock->get_fd();
```

我们这里先创建一个协程化的`socket`，然后再把这个协程化的`socket`放入`php_study_netstream_data_t`的`Socket *`中。我们会发现，这一步也是和`PHP`内核不同的地方。也就是协程化多出来的步骤。

然后，获取这个`socket`对应的`fd`值，存入`abstract->stream.socket`里面即可。这个存放`fd`值的步骤和`PHP`内核的做法一致。

我们继续分析代码：

```cpp
if (timeout)
{
    abstract->stream.timeout = *timeout;
}
else
{
    abstract->stream.timeout.tv_sec = -1;
}
```

存储`socket`操作的超时时间。（`timeout`这个功能我们还没有在协程化的`socket`中进行实现，后续我们会补上）

我们继续分析代码：

```cpp
stream = php_stream_alloc_rel(NULL, abstract, persistent_id, "r+");
if (stream == NULL)
{
    delete sock;
}
return stream;
```

这里，我们通过`php_stream_alloc_rel`创建了一个`php_stream`。我们可以看一看，函数`php_stream_alloc_rel`的第二个参数是`void *`类型。所以，这也是为什么我们可以在`php_study_netstream_data_t`中包含一个`Socket *`。

因为`php_stream_alloc_rel`函数在头文件`php_streams.h`里面声明的，所以，我们需要在`php_study.h`里面进行引入：

```cpp
#include "php_network.h"
#include "php_streams.h" // 新增的一行
```

`OK`，我们编译一下我们的扩展：

```shell
[root@7b6ef640478b study]# phpize --clean && phpize && ./configure && make install
----------------------------------------------------------------------
Installing shared extensions:     /usr/lib/php/extensions/debug-non-zts-20180731/
Installing header files:          /usr/include/php/
[root@7b6ef640478b study]#
```

`OK`，我们编译一下我们的扩展：

```shell
[root@7b6ef640478b study]# phpize --clean && phpize && ./configure && make install
----------------------------------------------------------------------
Installing shared extensions:     /usr/lib/php/extensions/debug-non-zts-20180731/
Installing header files:          /usr/include/php/
[root@7b6ef640478b study]#
```

编写测试脚本：

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
});

study_event_wait();
```

执行结果：

```shell
[root@7b6ef640478b study]# php test.php
Segmentation fault
```

出现了段错误。我们用`gdb`进行调试：

```shell
[root@7b6ef640478b study]# cgdb php
(gdb) r test.php

1318├───────> if (stream->ops->set_option) {
1319│                 ret = stream->ops->set_option(stream, option, value, ptrparam);
1320│         }
1321│
```

所以是`stream->ops`这个指针是`NULL`导致的。这是因为我们在创建`php_stream`的时候，没有指定导致的。我们指定一下：

```cpp
stream = php_stream_alloc_rel(&socket_ops, abstract, persistent_id, "r+");
```

这里，我们需要定义一下`socket_ops`这个**全局变量**：

```cpp
static php_stream_ops socket_ops
{
    NULL,
    NULL,
    NULL,
    NULL,
    "tcp_socket/coroutine",
    NULL, /* seek */
    NULL,
    NULL,
    socket_set_option,
};
```

我们需要实现`socket_set_option`：

```cpp
static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam);
static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam)
{
    return 0;
}
```

`OK`，我们编译一下我们的扩展：

```shell
[root@7b6ef640478b study]# phpize --clean && phpize && ./configure && make install
----------------------------------------------------------------------
Installing shared extensions:     /usr/lib/php/extensions/debug-non-zts-20180731/
Installing header files:          /usr/include/php/
[root@7b6ef640478b study]#
```

执行结果：

```shell
[root@7b6ef640478b study]# php test.php
Segmentation fault
[root@7b6ef640478b study]#
```

我们发现，出现了段错误。我们用`gdb`来进行调试：

```shell
[root@7b6ef640478b study]# cgdb php
(gdb) r test.php
Starting program: /usr/bin/php test.php
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib64/libthread_db.so.1".

Program received signal SIGSEGV, Segmentation fault.
0x0000000000000000 in ?? ()
Missing separate debuginfos, use: debuginfo-install cyrus-sasl-lib-2.1.26-23.el7.x86_64 glibc-2.17-292.el7.x86_64 keyutils-libs-1.5.8-3.el7.x86_64 krb5-libs-1.15.1-37.el7_7.2.x86_64 libcom_err-1.42.9-16.el7.x86_64 libcurl-7.29.0-54.el7.x8
6_64 libgcc-4.8.5-39.el7.x86_64 libidn-1.28-4.el7.x86_64 libselinux-2.5-14.1.el7.x86_64 libssh2-1.8.0-3.el7.x86_64 libstdc++-4.8.5-39.el7.x86_64 libxml2-2.9.1-6.el7_2.3.x86_64 nspr-4.21.0-1.el7.x86_64 nss-3.44.0-4.el7.x86_64 nss-softokn-f
reebl-3.44.0-5.el7.x86_64 nss-util-3.44.0-3.el7.x86_64 openldap-2.4.44-21.el7_6.x86_64 openssl-libs-1.0.2k-19.el7.x86_64 pcre-8.32-17.el7.x86_64 xz-libs-5.2.2-1.el7.x86_64 zlib-1.2.7-18.el7.x86_64
(gdb)

176│         if (failed) {
177│                 /* failure means that they don't get a stream to play with */
178│                 if (persistent_id) {
179│                         php_stream_pclose(stream);
180│                 } else {
181├───────────────────────> php_stream_close(stream);
182│                 }
183│                 stream = NULL;
184│         }
```

我们发现，在`php_stream_close`的时候发生了问题。这个原因是我们没有为`socket_ops`注册对应的`close`方法，导致`PHP`内核调用这个`close`方法的时候，出现了问题。我们需要注册一下。首先，修改`socket_ops`：

```cpp
static php_stream_ops socket_ops
{
    NULL,
    NULL,
    socket_close,
    NULL,
    "tcp_socket/coroutine",
    NULL, /* seek */
    NULL,
    NULL,
    socket_set_option,
};
```

然后实现`socket_close`：

```cpp
static int socket_close(php_stream *stream, int close_handle);
static int socket_close(php_stream *stream, int close_handle)
{
    php_study_netstream_data_t *abstract = (php_study_netstream_data_t *) stream->abstract;
    Socket *sock = (Socket*) abstract->socket;

    sock->close();
    delete sock;
    efree(abstract);
    return 0;
}
```

重新编译、安装扩展：

```shell
[root@7b6ef640478b study]# make clean && make install
----------------------------------------------------------------------
Installing shared extensions:     /usr/lib/php/extensions/debug-non-zts-20180731/
Installing header files:          /usr/include/php/
[root@7b6ef640478b study]#
```

再次执行脚本：

```shell
[root@7b6ef640478b study]# php test.php
resource(5) of type (stream)
[root@7b6ef640478b study]#
```

`OK`，符合预期。

[下一篇：如何bind和listen](./《PHP扩展开发》-协程-如何bind和listen.md)
