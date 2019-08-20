# server错误码

上一篇文章，我们实现了服务器数据的接收和发送。但是，我们的服务器什么时候可以停止向客户端发送数据呢？貌似我们还没有实现这个逻辑。我来举个例子，有如下测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

$sock = $serv->accept();
$buf = $serv->recv($sock);
$serv->send($sock, $buf);

$buf = $serv->recv($sock);
$serv->send($sock, $buf);

$buf = $serv->recv($sock);
$serv->send($sock, $buf);

```

这里，服务器进行了三次接收数据和发送数据的操作。我们执行脚本：

```cpp
~/codeDir/cppCode/study # php test.php 

```

然后，另起一个终端：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello
hello

```

此时，我们看一下服务器端，是没有任何异常输出的。现在，我们按下`Ctrl + c`中断掉客户端：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello
hello
^Cpunt!

~/codeDir/cppCode/study # 
```

此时看一下服务器端的输出：

```shell
~/codeDir/cppCode/study # php test.php 
[2019-08-20 02:12:11]	WARNING	stSocket_send: Error has occurred: (errno 32) Broken pipe in /root/codeDir/cppCode/study/src/socket.cc on line 110.

Warning: Study\Coroutine\Server::send(): send error in /root/codeDir/cppCode/study/test.php on line 13
~/codeDir/cppCode/study # 
```

我们发现，因为客户端的关闭，服务器在往客户端发送数据的时候，就会产生`Broken pipe`的错误。这个问题我们是需要去解决的。

我们知道，如果客户端断开了连接，那么在`C`层面上，服务器`recv`数据的时候，会得到返回值0，代表连接断开了。但是，我们来看看我们的接口设计：

```php
Study\Coroutine\Server->recv(int fd, int length = 65535): string | false;
```

也就是说，我们要么返回有效的字符串，要么就是返回`false`。所以，对于`false`，我们需要去区分是什么样的错误。我们可以借助错误码来实现，这其实和`C`的`errno`很类似。

首先，我们需要定义好我们的错误码以及错误码对应的错误内容。我们创建文件`include/error.h`，内容如下：

```cpp
#ifndef ERROR_H
#define ERROR_H

#include "study.h"

enum stErrorCode
{
    /**
     * connection error
     */
    ST_ERROR_SESSION_CLOSED_BY_SERVER = 1001,
    ST_ERROR_SESSION_CLOSED_BY_CLIENT,
};

const char* st_strerror(int code);

#endif	/* ERROR_H */
```

其中，

`ST_ERROR_SESSION_CLOSED_BY_SERVER`代表连接是被服务器关闭的，`ST_ERROR_SESSION_CLOSED_BY_CLIENT`代表连接是被客户端关闭的。`st_strerror`可以返回`errCode`对应的`errMsg`。

我们再创建文件`src/error.cc`内容如下：

```cpp
#include "error.h"
#include "log.h"

const char* st_strerror(int code)
{
    switch (code)
    {
    case ST_ERROR_SESSION_CLOSED_BY_SERVER:
        return "Session closed by server";
        break;
    case ST_ERROR_SESSION_CLOSED_BY_CLIENT:
        return "Session closed by client";
        break;
    default:
        snprintf(st_error, sizeof(st_error), "Unknown error: %d", code);
        return st_error;
        break;
    }
}
```

这段代码很简单，就是返回`errCode`对应的`errMsg`。然后，我们记得把`src/error.cc`加入到`config.m4`文件里面：

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
    src/socket.cc \
    src/log.cc \
    src/error.cc
"
```

然后重新生成一份`Makefile`文件：

```shell
~/codeDir/cppCode/study # phpize --clean && phpize && ./configure
```

然后，我们需要为我们的`Server`类增加属性`errCode`和`errMsg`：

```cpp
void study_coroutine_server_coro_init()
{
    // 省略了其他的代码
    zend_declare_property_long(study_coroutine_server_coro_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coroutine_server_coro_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);
}
```

接着，我们需要去对`stSocket_recv`的返回值进行判断，在接口`PHP_METHOD(study_coroutine_server_coro, recv)`里面：

```cpp
ret = stSocket_recv(fd, ZSTR_VAL(buf), length, 0);
if (ret == 0)
{
    zend_update_property_long(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("errCode"), ST_ERROR_SESSION_CLOSED_BY_CLIENT);
    zend_update_property_string(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("errMsg"), st_strerror(ST_ERROR_SESSION_CLOSED_BY_CLIENT));
		RETURN_FALSE;
}
```

我们增加了返回值为`0`的判断，说明客户端主动关闭了连接。我们记得在文件`include/study_server_coro.h`里面引入一下头文件`include/error.h`：

```cpp
#include "error.h"
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

$sock = $serv->accept();

while (1)
{
    $buf = $serv->recv($sock);
    if ($buf == false)
    {
        var_dump($serv->errCode);
        var_dump($serv->errMsg);
        break;
    }
    $serv->send($sock, $buf);
}
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 

```

另起一个终端：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello
hello
^Cpunt!

~/codeDir/cppCode/study # 
```

输入`hello`之后按`Ctrl + c`：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080
hello
hello
^Cpunt!

~/codeDir/cppCode/study # 
```

此时，服务器端会输出：

```shell
~/codeDir/cppCode/study # php test.php 
int(1002)
string(24) "Session closed by client"
~/codeDir/cppCode/study # 
```

`OK`，符合预期。