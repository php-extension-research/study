# 定义协程化的Socket类

我们发现，我们之前实现的那个服务器，它是非协程化的，所以，我们现在需要去协程化它。我们需要做的是协程化那些会被阻塞的`socket`函数。

我们创建文件`include/coroutine_socket.h`。因为我们这个协程化的`Socket`类带有的命名空间是：

```cpp
Study::Coroutine
```

这个命名空间会和我们的`Study::Coroutine`类冲突，所以，我们需要先修改我们的命名空间。我问了一下部门的大佬，说`cpp`的命名空间一般用小写，所以我这里统统改成小写。

首先是在文件`include/context.h`里面，我们需要改成：

```cpp
namespace study
{
class Context
{
};
}
```

也就是说，我把`Study`改成了小写的`study`。

然后是文件`include/coroutine.h`，需要改成：

```cpp
namespace study
{
class Coroutine
{
};
}
```

因为需要改动的地方比较多，所以其他地方大家照着样子修改即可。

然后，我们在文件`include/coroutine_socket.h`里面写代码：

```cpp
#ifndef COROUTINE_SOCKET_H
#define COROUTINE_SOCKET_H

#include "study.h"

namespace study { namespace coroutine {
class Socket
{
private:
    int sockfd;
public:
    Socket(int domain, int type, int protocol);
    Socket(int fd);
    ~Socket();
    int bind(int type, char *host, int port);
    int listen();
    int accept();
    ssize_t recv(void *buf, size_t len);
    ssize_t send(const void *buf, size_t len);
    int close();

    bool wait_event(int event);
};
}
}

#endif	/* COROUTINE_SOCKET_H */
```

这是我们协程化`Socket`最基本的函数了，我们实现完这些函数就可以协程化我们的服务器了。

然后，我们创建文件`src/coroutine/socket.cc`，这个文件用来实现`study::coroutine::Socket`里面的方法。里面的内容暂时如下：

```cpp
#include "coroutine_socket.h"
#include "socket.h"

using study::coroutine::Socket;

```

因为我们增加了这个需要被编译的源文件，所以我们需要修改`config.m4`文件：

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
    src/error.cc \
    src/core/base.cc \
    src/coroutine/socket.cc
"
```

`OK`，我们重新生成`Makefile`：

```shell
~/codeDir/cppCode/study # phpize --clean && phpize && ./configure
```

然后编译、安装扩展：

```shell
~/codeDir/cppCode/study # make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

编译成功，符合预期。

[下一篇：协程化Socket::Socket](./《PHP扩展开发》-协程-协程化Socket::Socket.md)





