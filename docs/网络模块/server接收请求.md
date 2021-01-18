# server接收请求

这篇文章，我们来实现一下服务器接受请求的逻辑。也就是说，我们需要实现如下接口：

```php
$clientfd = $serv->accept();
```

所以，我们需要为`Study\Coroutine\Server`类实现一个`accept`方法。这个方法我们不接收任何参数，所以我们先定义一个`void`参数，在文件`study_server_coro.cc`里面：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_void, 0, 0, 0)
ZEND_END_ARG_INFO()
```

然后我们去定义我们的`accept`方法：

```cpp
PHP_METHOD(study_coroutine_server_coro, accept)
{
    zval *zsock;
    int connfd;

    zsock = st_zend_read_property(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("sock"), 0);
    connfd = stSocket_accept(Z_LVAL_P(zsock));
    RETURN_LONG(connfd);
}
```

然后再注册这个方法：

```cpp
static const zend_function_entry study_coroutine_server_coro_methods[] =
{
    // 省略其他的注册方法
  
    PHP_ME(study_coroutine_server_coro, accept, arginfo_study_coroutine_void, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

其中，

`st_zend_read_property`是用来读取对象的某个属性的函数。我们这里需要读取服务器端的套接字`sock`，而`sock`是我们在调用`Server`的构造函数的时候创建并设置的（如果忘记了这一部分知识点，可以回顾一下`server`创建）。

`stSocket_accept`则是用来获取客户端的连接。

`RETURN_LONG(connfd);`把获取到的这个连接套接字返回给`PHP`脚本。

然后，我们在`php_study.h`文件里面定义`st_zend_read_property`函数：

```cpp
inline zval *st_zend_read_property(zend_class_entry *class_ptr, zval *obj, const char *s, int len, int silent)
{
    zval rv;
    return zend_read_property(class_ptr, obj, s, len, silent, &rv);
}
```

第一个参数`class_ptr`是需要获取的属性的这个对象的类指针，第二个参数`obj`是这个对象，`s`和`len`是这个属性的名字以及它的长度，我们通过宏`ZEND_STRL`就可以传递这两个参数。`silent`则是如果属性未定义，则禁止通知。

接着，我们在文件`socket.h`里面对`stSocket_accept`进行声明：

```cpp
int stSocket_accept(int sock);
```

然后在`src/socket.cc`里面对它进行实现：

```cpp
int stSocket_accept(int sock)
{
    int connfd;
    struct sockaddr_in sa;
    socklen_t len;

    len = sizeof(sa);
    connfd = accept(sock, (struct sockaddr *)&sa, &len);
    
    return connfd;
}
```

代码很简单，就是对`accept`进行了封装。

然后，我们对扩展重新进行编译和安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

编写测试代码：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
$sock = $serv->accept();
var_dump($sock);

```

逻辑很简单，就是创建一个服务器，然后去获取连接。

我们执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 

```

此时，服务器处于监听的状态，我们可以通过命令来查看一下：

```shell
~/codeDir/cppCode/study # netstat -antp
Active Internet connections (servers and established)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name    
tcp        0      0 127.0.0.11:45391        0.0.0.0:*               LISTEN      -
tcp        0      0 127.0.0.1:8080          0.0.0.0:*               LISTEN      2174/php
```

我们发现，`8080`端口处于监听的状态。

然后，我们去请求一下这个服务器：

```shell
~/codeDir/cppCode/study # nc 127.0.0.1 8080

```

我们再来看看服务器这端打印的内容：

```shell
~/codeDir/cppCode/study # php test.php 
int(4)
~/codeDir/cppCode/study # 
```

`OK`，符合预期，打印出了连接套接字。

[下一篇：server监听的封装](./《PHP扩展开发》-协程-server监听的封装.md)