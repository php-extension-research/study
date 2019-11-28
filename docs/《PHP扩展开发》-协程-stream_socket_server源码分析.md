# stream_socket_server源码分析

这篇文章我们开始来分析一下`php stream server`的源码，为后面我们`hook` `php stream server`做准备。`PHP`版本是`7.1.0`。

我们的调试代码如下：

```php
<?php

$socket = stream_socket_server(
    'tcp://0.0.0.0:6666',
    $errno, $errstr, STREAM_SERVER_BIND | STREAM_SERVER_LISTEN
);
```

开始调试：

```shell
sh-4.2# cgdb php
(gdb)
```

我们先在`php_init_stream_wrappers`处打一个断点：

```shell
(gdb) b php_init_stream_wrappers
Breakpoint 1 at 0x7df8aa: file /root/php-7.1.0/main/streams/streams.c, line 1651.
(gdb)
```

然后运行代码：

```shell
(gdb) r test.php
Starting program: /home/codes/php/php-7.1.0/output/bin/php test.php
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib64/libthread_db.so.1".

Breakpoint 1, php_init_stream_wrappers (module_number=0) at /root/php-7.1.0/main/streams/streams.c:1651
Missing separate debuginfos, use: debuginfo-install glibc-2.17-260.el7_6.4.x86_64 libxml2-2.9.1-6.el7_2.3.x86_64 nss-softokn-freebl-3.36.0-5.el7_5.x86_64 xz-libs-5.2.2-1.el7.x86_64 zlib-1.2.7-18.el7.x86_6
4
(gdb)

1649│ int php_init_stream_wrappers(int module_number)
1650│ {
1651├───────> le_stream = zend_register_list_destructors_ex(stream_resource_regular_dtor, NULL, "stream", module_number);
1652│         le_pstream = zend_register_list_destructors_ex(NULL, stream_resource_persistent_dtor, "persistent stream", module_number);
```

此时断点触发。

然后，我们看一下函数调用栈：

```shell
(gdb) bt
#0  php_init_stream_wrappers (module_number=0) at /root/php-7.1.0/main/streams/streams.c:1651
#1  0x00000000007bfe8b in php_module_startup (sf=0x10b8c20 <cli_sapi_module>, additional_modules=0x0, num_additional_modules=0) at /root/php-7.1.0/main/main.c:2227
#2  0x000000000092e814 in php_cli_startup (sapi_module=0x10b8c20 <cli_sapi_module>) at /root/php-7.1.0/sapi/cli/php_cli.c:424
#3  0x0000000000930767 in main (argc=2, argv=0x10dc280) at /root/php-7.1.0/sapi/cli/php_cli.c:1345
(gdb)
```

可以看到，在`PHP`启动的时候，会去调用`php_init_stream_wrappers`这个函数。我们继续看看`php_init_stream_wrappers`这个函数做了什么事情。运行到`1661`：

```shell
(gdb) u 1661

1661├───────> return (php_stream_xport_register("tcp", php_stream_generic_socket_factory) == SUCCESS
1662│                         &&
1663│                         php_stream_xport_register("udp", php_stream_generic_socket_factory) == SUCCESS
1664│ #if defined(AF_UNIX) && !(defined(PHP_WIN32) || defined(__riscos__) || defined(NETWARE))
1665│                         &&
1666│                         php_stream_xport_register("unix", php_stream_generic_socket_factory) == SUCCESS
1667│                         &&
1668│                         php_stream_xport_register("udg", php_stream_generic_socket_factory) == SUCCESS
1669│ #endif
1670│                 ) ? SUCCESS : FAILURE;
```

停在了执行`php_stream_xport_register`这个位置。根据名字我们大概可以猜出来，这个函数应该是去注册某个东西。我们进入这个函数：

```shell
(gdb) s
php_stream_xport_register (protocol=0xd770bf "tcp", factory=0x7f006f <php_stream_generic_socket_factory>) at /root/php-7.1.0/main/streams/transports.c:34
(gdb)

 32│ PHPAPI int php_stream_xport_register(const char *protocol, php_stream_transport_factory factory)
 33│ {
 34├───────> return zend_hash_str_update_ptr(&xport_hash, protocol, strlen(protocol), factory) ? SUCCESS : FAILURE;
 35│ }
```

可以发现，这个函数实际上就是更新一下`xport_hash`这个`hash`表，哈希表里面存的东西是`factory`。`factory`是`php_stream_transport_factory`类型。我们看一看`php_stream_transport_factory`具体是什么样子的。在文件`php_stream_transport.h`里面：

```c
typedef php_stream *(php_stream_transport_factory_func)(const char *proto, size_t protolen,
        const char *resourcename, size_t resourcenamelen,
        const char *persistent_id, int options, int flags,
        struct timeval *timeout,
        php_stream_context *context STREAMS_DC);
typedef php_stream_transport_factory_func *php_stream_transport_factory;
PHPAPI php_stream_transport_factory_func php_stream_generic_socket_factory;
```

可以看的，`php_stream_generic_socket_factory`是`php_stream_transport_factory_func`类型的变量。而`php_stream_transport_factory_func`是一个函数指针的类型。返回值是`php_stream *`，参数是：

```c
const char *proto, size_t protolen,
const char *resourcename, size_t resourcenamelen,
const char *persistent_id, int options, int flags,
struct timeval *timeout,
php_stream_context *context STREAMS_DC
```

所以，我们可以得出一个结论：php_stream_xport_register注册了一个函数指针，这个函数指针是`php_stream_transport_factory_func`类型的函数指针。

`OK`，我们继续：

```shell
(gdb) finish
Run till exit from #0  php_stream_xport_register (protocol=0xd770bf "tcp", factory=0x7f006f <php_stream_generic_socket_factory>) at /root/php-7.1.0/main/streams/transports.c:34
php_init_stream_wrappers (module_number=0) at /root/php-7.1.0/main/streams/streams.c:1670
Value returned is $1 = 0
(gdb)

1670├───────────────> ) ? SUCCESS : FAILURE;
1671│ }
```

所以，`php_init_stream_wrappers`的作用就是去注册`PHP`默认的`php_stream`函数。然后，我们在函数`zif_stream_socket_server`处打一个断点：

```shell
(gdb) b zif_stream_socket_server
Breakpoint 3 at 0x7a3c9a: file /root/php-7.1.0/ext/standard/streamsfuncs.c, line 179.
(gdb)
```

然后继续运行：

```shell
(gdb) c
Continuing.

Breakpoint 2, zif_stream_socket_server (execute_data=0x7ffff5e140d0, return_value=0x7ffff5e140b0) at /root/php-7.1.0/ext/standard/streamsfuncs.c:179
(gdb)

 175│ PHP_FUNCTION(stream_socket_server)
 176│ {
 177│         char *host;
 178│         size_t host_len;
 179├───────> zval *zerrno = NULL, *zerrstr = NULL, *zcontext = NULL;
```

此时，已经进入了我们`PHP`脚本写的`stream_socket_server`里面了。我们继续来看看这个函数实现了什么。我们运行到`207`行之前：

```shell
(gdb) u 207
zif_stream_socket_server (execute_data=0x7ffff5e140d0, return_value=0x7ffff5e140b0) at /root/php-7.1.0/ext/standard/streamsfuncs.c:207
(gdb)

 207├───────> stream = php_stream_xport_create(host, host_len, REPORT_ERRORS,
 208│                         STREAM_XPORT_SERVER | (int)flags,
 209│                         NULL, NULL, context, &errstr, &err);
```

可以看到，`php_stream_xport_create`函数会依据我们传入的`host`、`flag`等信息创建出一个`stream`。我们很容易的可以猜到，这个应该就是`php_stream *`类型的指针。我们进入`php_stream_xport_create`函数里面：

```shell
(gdb) s
_php_stream_xport_create (name=0x7ffff5e027e8 "tcp://0.0.0.0:6666", namelen=18, options=8, flags=13, persistent_id=0x0, timeout=0x0, context=0x7ffff5e01640, error_string=0x7fffffffb308, error_code=0x7ffff
fffb31c, __php_stream_call_depth=0, __zend_filename=0xd6ffb0 "/root/php-7.1.0/ext/standard/streamsfuncs.c", __zend_lineno=209, __zend_orig_filename=0x0, __zend_orig_lineno=0) at /root/php-7.1.0/main/strea
ms/transports.c:60
(gdb)

 52│ PHPAPI php_stream *_php_stream_xport_create(const char *name, size_t namelen, int options,
 53│                 int flags, const char *persistent_id,
 54│                 struct timeval *timeout,
 55│                 php_stream_context *context,
 56│                 zend_string **error_string,
 57│                 int *error_code
 58│                 STREAMS_DC)
 59│ {
 60├───────> php_stream *stream = NULL;
 61│         php_stream_transport_factory factory = NULL;
 62│         const char *p, *protocol = NULL;
```

可以看的，`php_stream_xport_create`确实会创建一个`php_stream`类型的指针。我们继续运行到`109`行之前：

```shell
(gdb) u 109
_php_stream_xport_create (name=0x7ffff5e027ee "0.0.0.0:6666", namelen=12, options=8, flags=13, persistent_id=0x0, timeout=0x7fffffffb230, context=0x7ffff5e01640, error_string=0x7fffffffb308, error_code=0x
7fffffffb31c, __php_stream_call_depth=0, __zend_filename=0xd6ffb0 "/root/php-7.1.0/ext/standard/streamsfuncs.c", __zend_lineno=209, __zend_orig_filename=0x0, __zend_orig_lineno=0) at /root/php-7.1.0/main/
streams/transports.c:109
(gdb)

109├───────> if (protocol) {
110│                 char *tmp = estrndup(protocol, n);
111│                 if (NULL == (factory = zend_hash_str_find_ptr(&xport_hash, tmp, n))) {
112│                         char wrapper_name[32];
```

我们发现，在`111`行会根据`protocol`的内容（在这个测试脚本中`protocol`是`tcp`）去`xport_hash`这个`hash`表里面查找`factory`，而这个`factory`应该就是我们之前注册的那个函数指针。我们继续执行到`126`行：

```shell
(gdb) u 126
_php_stream_xport_create (name=0x7ffff5e027ee "0.0.0.0:6666", namelen=12, options=8, flags=13, persistent_id=0x0, timeout=0x7fffffffb230, context=0x7ffff5e01640, error_string=0x7fffffffb308, error_code=0x
7fffffffb31c, __php_stream_call_depth=0, __zend_filename=0xd6ffb0 "/root/php-7.1.0/ext/standard/streamsfuncs.c", __zend_lineno=209, __zend_orig_filename=0x0, __zend_orig_lineno=0) at /root/php-7.1.0/main/
streams/transports.c:127
(gdb)

127├───────> if (factory == NULL) {
128│                 /* should never happen */
129│                 php_error_docref(NULL, E_WARNING, "Could not find a factory !?");
130│                 return NULL;
131│         }
132│
133│         stream = (factory)(protocol, n,
134│                         (char*)name, namelen, persistent_id, options, flags, timeout,
135│                         context STREAMS_REL_CC);
```

我们发现，在`133`行这个位置会去调用`factory`这个函数，并且创建出一个`stream`。我们继续运行，进入`factory`对应的函数里面：

```shell
(gdb) n
(gdb) s
php_stream_generic_socket_factory (proto=0x7ffff5e027e8 "tcp://0.0.0.0:6666", protolen=3, resourcename=0x7ffff5e027ee "0.0.0.0:6666", resourcenamelen=12, persistent_id=0x0, options=8, flags=13, timeout=0x
7fffffffb230, context=0x7ffff5e01640, __php_stream_call_depth=1, __zend_filename=0xd788d0 "/root/php-7.1.0/main/streams/transports.c", __zend_lineno=135, __zend_orig_filename=0xd6ffb0 "/root/php-7.1.0/ext
/standard/streamsfuncs.c", __zend_orig_lineno=209) at /root/php-7.1.0/main/streams/xp_socket.c:884
(gdb)

878│ PHPAPI php_stream *php_stream_generic_socket_factory(const char *proto, size_t protolen,
879│                 const char *resourcename, size_t resourcenamelen,
880│                 const char *persistent_id, int options, int flags,
881│                 struct timeval *timeout,
882│                 php_stream_context *context STREAMS_DC)
883│ {
884├───────> php_stream *stream = NULL;
```

我们发现，我们进入了在`php_init_stream_wrappers`函数中注册的`php_stream_generic_socket_factory`函数里面。

我们知道，是需要创建一个`socket`的，这样客户端才可以和服务器进行通信。那么，创建完`php_stream`之后，在哪个地方创建的`socket`呢？我们继续调试，退出`php_stream_generic_socket_factory`函数：

```shell
(gdb) finish
Run till exit from #0  php_stream_generic_socket_factory (proto=0x7ffff5e027e8 "tcp://0.0.0.0:6666", protolen=3, resourcename=0x7ffff5e027ee "0.0.0.0:6666", resourcenamelen=12, persistent_id=0x0, options=
8, flags=13, timeout=0x7fffffffb230, context=0x7ffff5e01640, __php_stream_call_depth=1, __zend_filename=0xd788d0 "/root/php-7.1.0/main/streams/transports.c", __zend_lineno=135, __zend_orig_filename=0xd6ff
b0 "/root/php-7.1.0/ext/standard/streamsfuncs.c", __zend_orig_lineno=209) at /root/php-7.1.0/main/streams/xp_socket.c:884
0x00000000007ed2f0 in _php_stream_xport_create (name=0x7ffff5e027ee "0.0.0.0:6666", namelen=12, options=8, flags=13, persistent_id=0x0, timeout=0x7fffffffb230, context=0x7ffff5e01640, error_string=0x7ffff
fffb308, error_code=0x7fffffffb31c, __php_stream_call_depth=0, __zend_filename=0xd6ffb0 "/root/php-7.1.0/ext/standard/streamsfuncs.c", __zend_lineno=209, __zend_orig_filename=0x0, __zend_orig_lineno=0) at
 /root/php-7.1.0/main/streams/transports.c:133
Value returned is $4 = (php_stream *) 0x7ffff5e5fa00
(gdb)

133├───────> stream = (factory)(protocol, n,
134│                         (char*)name, namelen, persistent_id, options, flags, timeout,
135│                         context STREAMS_REL_CC);
```

在`php_stream_xport_bind`处打一个断点：

```shell
(gdb) b php_stream_xport_bind
Breakpoint 5 at 0x7ed66c: file /root/php-7.1.0/main/streams/transports.c, line 205.
(gdb)
```

然后继续运行：

```shell
(gdb) c
Continuing.

Breakpoint 5, php_stream_xport_bind (stream=0x7ffff5e5fa00, name=0x7ffff5e027ee "0.0.0.0:6666", namelen=12, error_text=0x7fffffffb240) at /root/php-7.1.0/main/streams/transports.c:205
(gdb)

196│ /* Bind the stream to a local address */
197│ PHPAPI int php_stream_xport_bind(php_stream *stream,
198│                 const char *name, size_t namelen,
199│                 zend_string **error_text
200│                 )
201│ {
202│         php_stream_xport_param param;
203│         int ret;
204│
205├───────> memset(&param, 0, sizeof(param));
206│         param.op = STREAM_XPORT_OP_BIND;
207│         param.inputs.name = (char*)name;
208│         param.inputs.namelen = namelen;
209│         param.want_errortext = error_text ? 1 : 0;
210│
211│         ret = php_stream_set_option(stream, PHP_STREAM_OPTION_XPORT_API, 0, &param);
```

根据`php_stream_xport_bind`的注释，我们很容易知道，这个函数的作用肯定是会去调用`bind`函数来绑定`ip`和端口。并且我们发现，`php_stream_xport_bind`的核心函数就是`php_stream_set_option`。因为`php_stream_set_option`是一个宏，并且宏展开之后是`_php_stream_set_option`，所以我们在`_php_stream_set_option`处打一个断点：

```shell
(gdb) b _php_stream_set_option
Breakpoint 7 at 0x7dee15: file /root/php-7.1.0/main/streams/streams.c, line 1347.
(gdb)
```

继续运行：

```shell
(gdb) c
Continuing.

Breakpoint 7, _php_stream_set_option (stream=0x7ffff5e5fa00, option=7, value=0, ptrparam=0x7fffffffb110) at /root/php-7.1.0/main/streams/streams.c:1347
(gdb)

1345│ PHPAPI int _php_stream_set_option(php_stream *stream, int option, int value, void *ptrparam)
1346│ {
1347├───────> int ret = PHP_STREAM_OPTION_RETURN_NOTIMPL;
1348│
1349│         if (stream->ops->set_option) {
1350│                 ret = stream->ops->set_option(stream, option, value, ptrparam);
1351│         }
```

我们发现`_php_stream_set_option`函数的核心就是`stream->ops->set_option`，这个函数里面应该有我们希望看到的代码，我们进入这个函数：

```shell
(gdb) n
(gdb) n
(gdb) s

846│ static int php_tcp_sockop_set_option(php_stream *stream, int option, int value, void *ptrparam)
847│ {
848├───────> php_netstream_data_t *sock = (php_netstream_data_t*)stream->abstract;
849│         php_stream_xport_param *xparam;
```

我们发现进入了`php_tcp_sockop_set_option`函数里面，这个函数里面有一段核心的代码，通过`switch`语句根据`xparam->op`来选择执行：

```c
switch(option) {
    case PHP_STREAM_OPTION_XPORT_API:
        xparam = (php_stream_xport_param *)ptrparam;

        switch(xparam->op) {
            case STREAM_XPORT_OP_CONNECT:
            case STREAM_XPORT_OP_CONNECT_ASYNC:
                xparam->outputs.returncode = php_tcp_sockop_connect(stream, sock, xparam);
                return PHP_STREAM_OPTION_RETURN_OK;

            case STREAM_XPORT_OP_BIND:
                xparam->outputs.returncode = php_tcp_sockop_bind(stream, sock, xparam);
                return PHP_STREAM_OPTION_RETURN_OK;


            case STREAM_XPORT_OP_ACCEPT:
                xparam->outputs.returncode = php_tcp_sockop_accept(stream, sock, xparam STREAMS_CC);
                return PHP_STREAM_OPTION_RETURN_OK;
            default:
                /* fall through */
                ;
        }
}
```

在上面的调试过程中，我们知道，此时的`option`的值是`PHP_STREAM_OPTION_XPORT_API`。然后我们继续执行，看看会进入后面的那个`case`分支里面：

```shell
(gdb) n
(gdb) n
(gdb) n
(gdb) n
(gdb)
861│                                 case STREAM_XPORT_OP_BIND:
862├───────────────────────────────────────> xparam->outputs.returncode = php_tcp_sockop_bind(stream, sock, xparam);
863│                                         return PHP_STREAM_OPTION_RETURN_OK;
```

我们发现，进入了`STREAM_XPORT_OP_BIND`这个分支里面。我们进入函数`php_tcp_sockop_bind`里面：

```shell
(gdb) s
php_tcp_sockop_bind (stream=0x7ffff5e5fa00, sock=0x7ffff5e02870, xparam=0x7fffffffb110) at /root/php-7.1.0/main/streams/xp_socket.c:615
(gdb)

612│ static inline int php_tcp_sockop_bind(php_stream *stream, php_netstream_data_t *sock,
613│                 php_stream_xport_param *xparam)
614│ {
615├───────> char *host = NULL;
616│         int portno, err;
617│         long sockopts = STREAM_SOCKOP_NONE;
618│         zval *tmpzval = NULL;
619│
620│ #ifdef AF_UNIX
621│         if (stream->ops == &php_stream_unix_socket_ops || stream->ops == &php_stream_unixdg_socket_ops) {
622│                 struct sockaddr_un unix_addr;
623│
624│                 sock->socket = socket(PF_UNIX, stream->ops == &php_stream_unix_socket_ops ? SOCK_STREAM : SOCK_DGRAM, 0);
```

我们发现，函数`php_tcp_sockop_bind`里面有创建`socket`的代码。说明创建`socket`的代码被封装在了`php_tcp_sockop_bind`里面。并且在这个函数的后面，我们也看到了`bind`这个函数。

`OK`，我们现在分析完了`stream_socket_server`这个`PHP`函数的工作流程。我们在想，如果我们调用`php_stream_xport_register`去替换掉`xport_hash`里面保存的`php_stream_generic_socket_factory`函数指针，是不是就可以协程化了呢？
