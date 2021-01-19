# 压测server（一）

到目前为止，我们已经实现的`Server`的主要功能，可以接受请求和处理请求。但是，这是一个同步的`Server`。为了体现出同步`Server`和协程化的`Server`的差别，我们这一篇文章先来压测一下这个同步`Server`。

测试脚本如下：

```php
<?php

$serv = new Study\Coroutine\Server("127.0.0.1", 8080);

while (1)
{
    $connfd = $serv->accept();
    while (1)
    {
        $buf = $serv->recv($connfd);
        if ($buf == false)
        {
            break;
        }
        $serv->send($connfd, "hello");
    }
}
```

这是一个单进程单线程的同步`Server`。代码很简单，接收客户端的连接、接收客户端发来的数据，发送数据给客户端。

然后，我们使用`Swoole`提供的压测脚本来进行压测，在`swoole/benchmark/`下的 [main.php](https://github.com/swoole/benchmark/blob/master/main.php) 文件。我们先启动我们的服务器：

```php
~/codeDir/cppCode/study # php test.php 

```

然后执行压测脚本，

我们进行第一次压测：

```shell
~/codeDir/cppCode/swoole-src-analysis/benchmark # php main.php run -c 100 -r 100000 -l -s 127.0.0.1:8080 tcp


Concurrency Level:      100
Time taken for tests:   4.5937 seconds
Complete requests:      100,000
Failed requests:        0
Connect failed:         0
Total send:             102,400,000 bytes
Total reveive:          500,000 bytes
Requests per second:    21768.94
Connection time:        4.5937 seconds

~/codeDir/cppCode/swoole-src-analysis/benchmark # 
```

连接数为`100`，总的请求数为`100000`，`qps`可以达到`21768.94`，还算满意。

我们进行第二次压测：

```shell
~/codeDir/cppCode/swoole-src-analysis/benchmark # php main.php run -c 1000 -r 100000 -l -s 127.0.0.1:8080 tcp


Concurrency Level:      1000
Time taken for tests:   1.0417 seconds
Complete requests:      100,000
Failed requests:        86,800
Connect failed:         868
Total send:             13,516,800 bytes
Total reveive:          66,000 bytes
Requests per second:    12671.59
Connection time:        1.0417 seconds

~/codeDir/cppCode/swoole-src-analysis/benchmark # 
```

连接数为`1000`，总的请求数为`100000`，`qps`可以达到`12671.59`，还算满意吧。

我们进行第三次压测：

```shell
~/codeDir/cppCode/swoole-src-analysis/benchmark # php main.php run -c 10000 -r 100000 -l -s 127.0.0.1:8080 tcp


Concurrency Level:      10000
Time taken for tests:   2.8066 seconds
Complete requests:      100,000
Failed requests:        96,670
Connect failed:         9,667
Total send:             3,409,920 bytes
Total reveive:          16,650 bytes
Requests per second:    1186.49
Connection time:        2.8066 seconds

~/codeDir/cppCode/swoole-src-analysis/benchmark # 
```

连接数为`10000`，总的请求数为`100000`，`qps`可以达到`1186.49`，这就比较尴尬了对吧。

为什么会这样呢？因为我们的服务器是同步的，必须处理完第一个连接的所有请求，才可以去处理其他的连接。那么就会有这种情况：如果其中一个连接阻塞了，那么就会让总的时间上去，而且其他连接也是得不到处理的，所以`qps`就会降低。

那么，如果我们让服务器协程化的话，就可以做到一个连接是阻塞的，可以切换协程去处理其他的连接，这样我们就可以让`CPU`一直服务于我们的服务器，这样，就可以利用一个连接阻塞的时间去处理其他的连接，会让整个压测的时间大大降低。

[下一篇：socket可读写时候调度协程的思路](./《PHP扩展开发》-协程-socket可读写时候调度协程的思路.md)

