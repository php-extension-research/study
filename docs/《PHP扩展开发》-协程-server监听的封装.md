# server监听的封装

在编写代码的时候，我发现我们对`socket`创建、`socket`绑定、`socket accept`都有一个简单的封装，但是对于`listen`就没有封装：

```cpp
listen(sock, 512);
```

所以，我这里简单的封装一下。先在`include/socket.h`文件里面进行函数的声明：

```cpp
int stSocket_listen(int sock);
```

然后在文件`src/socket.cc`里面进行实现：

```cpp
int stSocket_listen(int sock)
{
    int ret;

    ret = listen(sock, 512);
    if (ret < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    return ret;
}
```

其中，`stWarn`是一个日志打印的宏，这个地方我们就不讲解了，大家直接去我的`github`上面复制下`include/log.h`和`src/log.cc`就好了。记得修改`config.m4`文件，增加`src/log.cc`为需要编译的文件。

然后我们修改接口的调用，在`PHP_METHOD(study_coroutine_server_coro, __construct)`接口里面：

```cpp
sock = stSocket_create(ST_SOCK_TCP);
stSocket_bind(sock, ST_SOCK_TCP, Z_STRVAL_P(zhost), zport);
stSocket_listen(sock); // 修改的地方
```

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
var_dump($serv);
```

然后执行：

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

`OK`，符合预期。