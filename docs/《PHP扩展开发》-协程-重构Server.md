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
