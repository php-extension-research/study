# 保存PHP栈

我们现在来压测一下我们的服务器，测试脚本如下：

```php
<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
        }, $serv, $connfd);
    }
});

study_event_wait();
```

这个服务器很简单，一个短连接服务器，接收一个连接之后，就另起一个协程去处理这个连接，处理完连接立马断开连接。

我们启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
This is ApacheBench, Version 2.3 <$Revision: 1843412 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests

Test aborted after 10 failures

apr_socket_connect(): Connection reset by peer (104)
```

报错，并且服务器这边出现段错误：

```shell
Segmentation fault
~/codeDir/cppCode/study #
```

我们用`gdb`调试：

```shell
Program received signal SIGSEGV, Segmentation fault.
0x0000555555a20588 in ?? ()
(gdb) bt
#0  0x0000555555a20588 in ?? ()
#1  0x00007ffff78d137f in study::PHPCoroutine::create_func (arg=<optimized out>) at /root/codeDir/cppCode/study/study_coroutine.cc:113
#2  0x00007ffff78d259b in study::Context::context_func (arg=0x55555667e4f0) at /root/codeDir/cppCode/study/src/coroutine/context.cc:51
#3  0x00007ffff78d25d1 in make_fcontext () at /root/codeDir/cppCode/study/thirdparty/boost/asm/make_x86_64_sysv_elf_gas.S:64
#4  0x0000000000000000 in ?? ()
(gdb)
```

发现报错地方是：

```cpp
zend_vm_stack stack = EG(vm_stack);
efree(stack); // 出问题的地方
```

释放协程栈出了问题。这个问题是我们在切换协程上下文的时候，没有正确的处理`PHP`栈导致的。因为`PHP`作为一门语言，是有自己的执行流的。所以，我们需要处理这个执行流。

在文件`study_coroutine.h`里面：

```cpp
public:
    static void init();
    static inline php_coro_task* get_origin_task(php_coro_task *task)
    {
        Coroutine *co = task->co->get_origin();
        return co ? (php_coro_task *) co->get_task() : &main_task;
    }

protected:
    static void on_yield(void *arg);
    static void on_resume(void *arg);
    static inline void restore_task(php_coro_task *task);
    static inline void restore_vm_stack(php_coro_task *task);
```

其中，

`get_origin_task`获取上一个任务的`task`结构。

`on_yield`会在协程被`yield`的时候，去调用保存`PHP`栈、加载`PHP`栈的方法。

`on_resume`会在协程被`resume`的时候，去调用保存`PHP`栈、加载`PHP`栈的方法。

`restore_vm_stack`用来重新加载`PHP`栈。

然后，我们在文件`study_coroutine.cc`里面去实现这些方法，先引入头文件`coroutine.h`：

```cpp
#include "coroutine.h"
```

然后去实现方法：

```cpp
void PHPCoroutine::on_yield(void *arg)
{
    php_coro_task *task = (php_coro_task *) arg;
    php_coro_task *origin_task = get_origin_task(task);
    save_task(task);
    restore_task(origin_task);
}

void PHPCoroutine::on_resume(void *arg)
{
    php_coro_task *task = (php_coro_task *) arg;
    php_coro_task *current_task = get_task();
    save_task(current_task);
    restore_task(task);
}

/**
 * load PHP stack
 */
void PHPCoroutine::restore_task(php_coro_task *task)
{
    restore_vm_stack(task);
}

/**
 * load PHP stack
 */
inline void PHPCoroutine::restore_vm_stack(php_coro_task *task)
{
    EG(vm_stack_top) = task->vm_stack_top;
    EG(vm_stack_end) = task->vm_stack_end;
    EG(vm_stack) = task->vm_stack;
    EG(vm_stack_page_size) = task->vm_stack_page_size;
    EG(current_execute_data) = task->execute_data;
}

void PHPCoroutine::init()
{
    Coroutine::set_on_yield(on_yield);
    Coroutine::set_on_resume(on_resume);
}
```

代码意图比较简单，和保存`PHP`栈是一个互逆的操作，不多啰嗦。

`PHPCoroutine::init`就是去设置保存、加载`PHP`栈的回调函数。

然后，我们在模块初始化方法`study_coroutine_util_init`里面去调用`init`方法：

```cpp
void study_coroutine_util_init()
{
    PHPCoroutine::init(); // 新增的一行
    INIT_NS_CLASS_ENTRY(study_coroutine_ce, "Study", "Coroutine", study_coroutine_util_methods);
```

然后我们在文件`include/coroutine.h`里面：

```cpp
typedef void (*st_coro_on_swap_t)(void*);
```

定义一个函数指针。

然后在类`study::Coroutine`里面声明：

```cpp
public:
    static void set_on_yield(st_coro_on_swap_t func);
    static void set_on_resume(st_coro_on_swap_t func);

    inline Coroutine* get_origin()
    {
        return origin;
    }

protected:
    static st_coro_on_swap_t on_yield;
    static st_coro_on_swap_t on_resume;
```

其中，`on_yield`和`on_resume`用来保存函数指针。实际上就是`study::PHPCoroutine::on_yield`和`study::PHPCoroutine::on_resume`。

然后在文件`src/coroutine/coroutine.cc`里面初始化类`static`变量：

```cpp
st_coro_on_swap_t Coroutine::on_yield = nullptr;
st_coro_on_swap_t Coroutine::on_resume = nullptr;
```

接着去实现`set_on_yield`和`set_on_resume`。

```cpp
void Coroutine::set_on_yield(st_coro_on_swap_t func)
{
    on_yield = func;
}

void Coroutine::set_on_resume(st_coro_on_swap_t func)
{
    on_resume = func;
}
```

`OK`，我们保存完了`PHP`栈，现在重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后重新执行服务器脚本：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   0.795 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    12573.00 [#/sec] (mean)
Time per request:       7.954 [ms] (mean)
Time per request:       0.080 [ms] (mean, across all concurrent requests)
Transfer rate:          1178.72 [Kbytes/sec] received
```

`qps`达到了`12573.00`还是比较满意的。

我们继续加大并发连接数以及请求量：

```shell
~/codeDir/cppCode/study # ab -c 1000 -n 100000 127.0.0.1:8080/
Concurrency Level:      1000
Time taken for tests:   7.530 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      9600000 bytes
HTML transferred:       1300000 bytes
Requests per second:    13280.85 [#/sec] (mean)
Time per request:       75.296 [ms] (mean)
Time per request:       0.075 [ms] (mean, across all concurrent requests)
Transfer rate:          1245.08 [Kbytes/sec] received
```

我们再次加大并发连接数以及请求量：

```shell
~/codeDir/cppCode/study # ab -c 10000 -n 1000000 127.0.0.1:8080/
Concurrency Level:      10000
Time taken for tests:   80.739 seconds
Complete requests:      1000000
Failed requests:        0
Total transferred:      96000000 bytes
HTML transferred:       13000000 bytes
Requests per second:    12385.57 [#/sec] (mean)
Time per request:       807.391 [ms] (mean)
Time per request:       0.081 [ms] (mean, across all concurrent requests)
Transfer rate:          1161.15 [Kbytes/sec] received
```

我们发现，在`1W`连接，`100W`的请求量下面，我们的服务器的`qps`依然很稳定。

我们来模拟业务阻塞的情况。修改测试脚本：

```php
<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            usleep(10);
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
        }, $serv, $connfd);
    }
});

study_event_wait();
```

我们在获取到连接的时候，去睡眠`10`毫秒。我们启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   1.605 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    6231.19 [#/sec] (mean)
Time per request:       16.048 [ms] (mean)
Time per request:       0.160 [ms] (mean, across all concurrent requests)
Transfer rate:          584.17 [Kbytes/sec] received
```

我们发现`qps`降低到了`6231.19`。因为，`usleep`是一个同步阻塞的函数，会阻塞住整个线程，所以，我们的所有协程都会被阻塞住。

我们现在使用协程化的阻塞`sleep`。修改测试脚本：

```php
<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            Sco::sleep(0.01);
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
        }, $serv, $connfd);
    }
});

study_event_wait();
```

重新启动服务器脚本：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   1.261 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    7933.26 [#/sec] (mean)
Time per request:       12.605 [ms] (mean)
Time per request:       0.126 [ms] (mean, across all concurrent requests)
Transfer rate:          743.74 [Kbytes/sec] received
```

我们发现`qps`达到了`8000`。这个不明显因为，我们直接在创建子协程的入口就调用了`co::sleep`。所以，在创建完**所有**的协程之前，`ab`压测工具都是无法收到服务器的响应的。我们需要修改测试脚本：

```php
<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
            usleep(10);
        }, $serv, $connfd);
    }
});

study_event_wait();
```

我们在完成每次请求的时候，再阻塞。我们启动服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

然后进行压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   1.709 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    5850.24 [#/sec] (mean)
Time per request:       17.093 [ms] (mean)
Time per request:       0.171 [ms] (mean, across all concurrent requests)
Transfer rate:          548.46 [Kbytes/sec] received
```

我们发现，`qps`依然还是`5850.24`那么低。

我们修改从协程化的`sleep`：

```php
<?php

study_event_init();

Sgo(function ()
{
    $serv = new Study\Coroutine\Server("127.0.0.1", 8080);
    while (1)
    {
        $connfd = $serv->accept();

        Sgo(function ($serv, $connfd)
        {
            $buf = $serv->recv($connfd);
            $responseStr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nContent-Length: 11\r\n\r\nhello world\r\n";
            $serv->send($connfd, $responseStr);
            $serv->close($connfd);
            Sco::sleep(0.01);
        }, $serv, $connfd);
    }
});

study_event_wait();
```

重启服务器：

```shell
~/codeDir/cppCode/study # php test.php

```

重新压测：

```shell
~/codeDir/cppCode/study # ab -c 100 -n 10000 127.0.0.1:8080/
Concurrency Level:      100
Time taken for tests:   0.692 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      960000 bytes
HTML transferred:       130000 bytes
Requests per second:    14453.56 [#/sec] (mean)
Time per request:       6.919 [ms] (mean)
Time per request:       0.069 [ms] (mean, across all concurrent requests)
Transfer rate:          1355.02 [Kbytes/sec] received
```

可以看到，`qps`达到了`14453.56`。

**这，就是协程的威力。**
