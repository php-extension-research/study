# sleep（五）

这一篇文章，我们来优化一下我们的调度器。首先，我们来回顾一下我们的调度器代码：

```cpp
int PHPCoroutine::scheduler()
{
    uv_loop_t* loop = uv_default_loop();

    while (loop->stop_flag == 0)
    {
        loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
        uv__run_timers(loop);

        if (uv__next_timeout(loop) < 0)
        {
            uv_stop(loop);
        }
    }

    return 0;
}
```

我们发现这是一个死循环。那这又意味着什么呢？

意味着如果如果`loop->stop_flag`等于`0`，那么，这段`while`循环就会一直的执行下去，我们的`CPU`就会一直被占用。而且，如果定时器节点它一直没有过期，那么这个`while`循环就会无止尽的做无用功。

我们来测试一下，测试脚本如下：

```php
<?php

Sgo(function () {
    echo "co1 before sleep" . PHP_EOL;
    SCo::sleep(1000);
    echo "co1 after sleep" . PHP_EOL;
});

Sgo(function () {
    echo "co2 before sleep" . PHP_EOL;
    SCo::sleep(2000);
    echo "co2 after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;

SCo::scheduler();
```

我们开了两个协程，分别让第一、二个协程阻塞`1000s`和`2000s`。我们执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
co1 before sleep
co2 before sleep
main co

```

然后执行`top`命令：

```shell
~/codeDir/cppCode/study # top -b
Mem: 1908496K used, 138436K free, 11564K shrd, 261188K buff, 1282904K cached
CPU:   0% usr  50% sys   0% nic  50% idle   0% io   0% irq   0% sirq
Load average: 0.62 0.22 0.07 3/505 32731
  PID  PPID USER     STAT   VSZ %VSZ CPU %CPU COMMAND
32729 31958 root     R    37788   2%   0  50% php test.php
```

我们发现，`STAT`是`R`，也就是代表这个线程一直在占用着`CPU`，这显然是我们不愿意看到的。所以我们需要优化掉。

那么，什么情况下这个`loop->stop_flag`会一直等于`0`呢？当我们的定时器堆里面还有定时器节点的时候。

那么我们是否有什么优化的方法呢？有的，我们可以根据定时器堆顶的那个定时器节点来让我们整个线程阻塞起来，这样就不会一直占用`CPU`了。这里，我们可以借助`usleep`函数来做到，修改`PHPCoroutine::scheduler`代码如下：

```cpp
int PHPCoroutine::scheduler()
{
    int timeout; // 增加的代码
    uv_loop_t* loop = uv_default_loop();

    while (loop->stop_flag == 0)
    {
        timeout = uv__next_timeout(loop); // 增加的代码
        usleep(timeout); // 增加的代码
        
        loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
        uv__run_timers(loop);

        if (uv__next_timeout(loop) < 0)
        {
            uv_stop(loop);
        }
    }

    return 0;
}
```

`OK`，重新编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

执行脚本：

```php
<?php

Sgo(function () {
    echo "co1 before sleep" . PHP_EOL;
    SCo::sleep(1);
    echo "co1 after sleep" . PHP_EOL;
});

Sgo(function () {
    echo "co2 before sleep" . PHP_EOL;
    SCo::sleep(2);
    echo "co2 after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;

SCo::scheduler();
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php 
co1 before sleep
co2 before sleep
main co
co1 after sleep
co2 after sleep
~/codeDir/cppCode/study # 
```

我们会发现，在`main co`输出之后隔了`1s`输出了`co1 after sleep`。然后再隔了`1s`输出了`co2 after sleep`。

说明我们修改完代码之后，没有影响`sleep`接口。那么，我们来测试一下`CPU`的占用情况，我们先修改一下测试脚本：

```php
<?php

Sgo(function () {
    echo "co1 before sleep" . PHP_EOL;
    SCo::sleep(1000);
    echo "co1 after sleep" . PHP_EOL;
});

Sgo(function () {
    echo "co2 before sleep" . PHP_EOL;
    SCo::sleep(2000);
    echo "co2 after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;

SCo::scheduler();
```

然后执行：

```shell
~/codeDir/cppCode/study # php test.php 
co1 before sleep
co2 before sleep
main co

```

然后执行`top`命令：

```shell
~/codeDir/cppCode/study # top -b
Mem: 1910588K used, 136344K free, 11564K shrd, 261208K buff, 1282916K cached
CPU:   0% usr   4% sys   0% nic  95% idle   0% io   0% irq   0% sirq
Load average: 0.42 0.22 0.11 1/504 34960
  PID  PPID USER     STAT   VSZ %VSZ CPU %CPU COMMAND
34959 31958 root     S    37788   2%   1   0% php test.php
```

我们发现，此时这个线程处于`S`状态，也就是`sleep`状态，这个时候就不会去占用`CPU`了，符合我们的要求。

[下一篇：server创建（一）](./《PHP扩展开发》-协程-server创建（一）.md)