# 协程化服务器（二）

这篇文章，我们来协程化我们的

```cpp
PHP_METHOD(study_coroutine_server_coro, accept)
```

接口，代码修改如下：

```cpp
PHP_METHOD(study_coroutine_server_coro, accept)
{
    zval *zsock;
    Socket *sock; // 新增的一行
    int connfd;

    zsock = st_zend_read_property(study_coroutine_server_coro_ce_ptr, getThis(), ZEND_STRL("zsock"), 0); // 修改的一行
    sock = (Socket *)Z_PTR_P(zsock); // 修改的一行
    connfd = sock->accept(); // 修改的一行
    RETURN_LONG(connfd);
}
```

其中，

我们通过

```cpp
st_zend_read_property
```

读取到我们在构造函数里面设置的`Socket`对象。然后，我们再调用这个对象的`accept`方法。

重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

符合预期。

然后编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);
$connfd = $serv->accept();
var_dump($connfd);
```

代码很简单，我们启动一个服务器之后，就尝试去`accept`一个连接。

我们启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 
[2019-09-05 04:53:35]	WARNING	stSocket_accept: Error has occurred: (errno 11) Resource temporarily unavailable in /root/codeDir/cppCode/study/src/socket.cc on line 56.
Segmentation fault
~/codeDir/cppCode/study # 
```

这里有两个问题，第一个是报出了一个警告，对应`errno 11`也就是`EAGAIN`错误码。我们看一下`src/socket.cc` `56`行的位置：

```cpp
connfd = accept(sock, (struct sockaddr *)&sa, &len);
if (connfd < 0)
{
  	stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
}
```

代码是这样的。因为我们的协程尝试着去获取一个客户端的连接，但是此时没有客户端连接，导致`connfd`的返回值是`-1`，因此报错了。但是，我们显然不希望这个报错，所以我们修改代码如下：

```cpp
if (connfd < 0 && errno != EAGAIN)
{
  	stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
}
```

同样的，在`recv`和`send`这两个阻塞的函数里面也有这个问题，所以我们也是需要这样修改的：

```cpp
if (ret < 0 && errno != EAGAIN)
{
  	stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
}
```

然后，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 
Segmentation fault
~/codeDir/cppCode/study # 
```

警告消失。现在，我们来解决一下这个段错误的问题。

我们打开`gdb`进行调试：

```shell
~/codeDir/cppCode/study # cgdb php
GNU gdb (GDB) 8.2
Copyright (C) 2018 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-alpine-linux-musl".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from php...(no debugging symbols found)...done.
(gdb) r test.php 

77│ bool Socket::wait_event(int event)
78│ {
79│     long id;
80│     Coroutine* co;
81│     epoll_event *ev;
82│
83├───> co = Coroutine::get_current();
84│     id = co->get_cid();
85│
86│     ev = StudyG.poll->events;
87│
88│     ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
89│     ev->data.u64 = touint64(sockfd, id);
90│     epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_ADD, sockfd, ev);
91│
92│     co->yield();
93│     return true;
94│ }
```

我们发现，在`83`行，获取当前协程的时候出现了问题。也就是说，当我们调用`Socket::accept`的时候，切换当前协程的时候出现了问题。

为什么呢？因为我们现在处于非协程环境里面，我们修改服务器脚本如下：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    $connfd = $serv->accept();
    var_dump($connfd);
});
```

把服务器包在了一个协程的环境里面。

再次启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 
Segmentation fault
~/codeDir/cppCode/study # 
```

还是有段错误。我们再次进行调试：

```shell
~/codeDir/cppCode/study # cgdb php
GNU gdb (GDB) 8.2
Copyright (C) 2018 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-alpine-linux-musl".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from php...(no debugging symbols found)...done.
(gdb) r test.php 


77│ bool Socket::wait_event(int event)
78│ {
79│     long id;
80│     Coroutine* co;
81│     epoll_event *ev;
82│
83│     co = Coroutine::get_current();
84│     id = co->get_cid();
85│
86├───> ev = StudyG.poll->events;
87│
88│     ev->events = event == ST_EVENT_READ ? EPOLLIN : EPOLLOUT;
89│     ev->data.u64 = touint64(sockfd, id);
90│     epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_ADD, sockfd, ev);
91│
92│     co->yield();
93│     return true;
94│ }
```

我们发现在获取`StudyG.poll->events`的时候出现了问题。为什么呢？因为我们的`StudyG.poll`是在调度器里面进行初始化的。而`wait_event`比调度器优先执行，所以，在调用`wait_event`的时候，`StudyG.poll`实际上还未被初始化。

所以，我们需要修改这里的代码：

```cpp
if (!StudyG.poll)
{
  	init_stPoll();
}

ev = StudyG.poll->events;
```

我们在使用`StudyG.poll`之前做一个判断即可。

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

然后再次启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 
~/codeDir/cppCode/study # 
```

此时，我们的服务器直接退出了。为什么呢？因为我们没有调用我们的调度器，调度器是无限循环，并且会在事件触发的时候调度对应的协程。我们修改服务器脚本如下：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    $connfd = $serv->accept();
    var_dump($connfd);
});

Sco::scheduler();
```

然后重新启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 

```

此时，服务器处于监听状态。

然后，我们用客户端去连接它：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080

```

此时，服务器打印出如下内容：

```shell
~/codeDir/cppCode/study # php test.php 
int(11)

```

`OK`，符合预期。

我们断开连接，再次连接一次：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test # nc 127.0.0.1 8080

```

然后，服务端打印出：

```shell
~/codeDir/cppCode/study # php test.php 
int(11)
Segmentation fault
~/codeDir/cppCode/study # 
```

报错了。

因为我们的服务器脚本没有循环的去获取连接，所以，我们需要修改服务器脚本：

```php
<?php

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();
        var_dump($connfd);
    }
});

Sco::scheduler();
```

重新启动服务器：

```shell
~/codeDir/cppCode/study # php test.php 

```

然后两次连接服务器：

```shell
~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test # nc 127.0.0.1 8080
^Cpunt!

~/codeDir/cppCode/test # 
```

此时，服务器端打印出：

```shell
~/codeDir/cppCode/study # php test.php 
int(11)
int(12)

```

符合预期。

(因为我们在服务器端并没有`close`掉服务器，所以此时连接套接字的数值是不断的递增的，我们后面会去处理一些列的问题，包括性能优化上面，内存泄漏等等问题)

