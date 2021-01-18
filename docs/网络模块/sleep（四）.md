# sleep（四）

这一篇文章，我们开始来完善一下我们的`PHPCoroutine::scheduler`方法。

首先我们来回顾一下在`Coroutine::sleep`里面的代码：

```cpp
uv_timer_t timer;
timer.data = co;
uv_timer_init(uv_default_loop(), &timer);
uv_timer_start(&timer, sleep_timeout, seconds * 1000, 0);
```

我们发现，这个代码段不足以开启定时器，这个仅仅是把定时器节点插入了定时器堆里面。如果我们要启动这个定时器，我们还需要执行`uv_run`。但是，我们不可以在`Coroutine::sleep`里面去调用`uv_run`，因为一旦我们调用了这个方法，那么，我们就会立马进入`libuv`的事件循环，这显然是不合理的。所以，我们需要自己去实现出`uv_run`的效果。而这个效果，我们就在`PHPCoroutine::scheduler`里面去实现它。

`OK`，我们现在来实现一下。

我们修改`PHPCoroutine::scheduler`这个方法的代码：

```cpp
typedef enum {
    UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
    UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

extern "C" void uv__run_timers(uv_loop_t* loop);
extern "C" uint64_t uv__hrtime(uv_clocktype_t type);
extern "C" int uv__next_timeout(const uv_loop_t* loop);

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

因为`libuv`在它`src`目录下面的头文件不会被安装在`/usr/local/include`这个目录项目，所以，如果我们不重复定义`uv_clocktype_t`以及`extern`这三个函数（`uv__run_timers`、`uv__hrtime`、`uv__next_timeout`），那么在编译的时候编译器就会说没有声明这些函数。这是我使用`libuv`比较头疼的一件事情。

我们来分析一下这段代码，其中：

```cpp
uv_loop_t* loop = uv_default_loop();
```

用来获取到`libuv`这个库里面定义好的全局变量`loop`，我们可以通过这个变量来控制循环。

```cpp
while (loop->stop_flag == 0)
```

用来判断循环是否结束。

```cpp
loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
```

修改当前的时间。

```cpp
uv__run_timers(loop);
```

这个函数会遍历整个定时器堆，让我们设置的每个定时器节点时间和`loop->time`进行比较，如果这个定时器节点的时间大于了这个`loop->time`，也就意味着定时器过期了，这个时候，就会去执行这个定时器节点的回调函数。而这个回调函数是我们自己设置的，我们可以回顾一下我们写好的`Coroutine::sleep`代码：

```cpp
uv_timer_start(&timer, sleep_timeout, seconds * 1000, 0);
```

其中`sleep_timeout`就是我们的回调函数：

```cpp
static void sleep_timeout(uv_timer_t *timer)
{
    ((Coroutine *) timer->data)->resume();
}
```

这个函数的作用就是去`resume`当前协程，于是就起到了唤醒当前协程的效果。

我们继续分析`PHPCoroutine::scheduler`的代码。

```cpp
if (uv__next_timeout(loop) < 0)
{
 		uv_stop(loop);
}
```

这段代码的作用是先执行`uv__next_timeout`来判断一下是否还有未执行的定时器，如果没有，那么会返回`-1`，然后执行`uv_stop`把`loop->stop_flag`设置为1，结束我们最外层的`while`循环。

`OK`，我们来重新编译、安装一下：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

$t1 = time();

$cid = Sgo(function () {
    echo "before sleep" . PHP_EOL;
    SCo::sleep(1);
    echo "after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;

SCo::scheduler();
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php 
before sleep
main co
after sleep
~/codeDir/cppCode/study # 
```

我们会发现输出`main co`之后会阻塞`1s`当前协程，然后再输出`after sleep`。这样，我们就实现完了协程的`sleep`接口。

[下一篇：sleep（五）](./《PHP扩展开发》-协程-sleep（五）.md)





