# sleep（二）

这篇文章我们继续去实现`PHPCoroutine::sleep`。

因为，我们的`PHP`协程是通过更加底层的（通用的）`Coroutine`实现的，所以，我们需要去实现`Coroutine`类里面的`sleep`方法。

我们现在`include/coroutine.h`文件中的`Coroutine`中声明这个方法：

```cpp
public:
    static int sleep(double seconds);
```

然后在`src/coroutine/coroutine.cc`里面去实现它：

```cpp
static void sleep_timeout(uv_timer_t *timer)
{
    ((Coroutine *) timer->data)->resume();
}

int Coroutine::sleep(double seconds)
{
    Coroutine* co = Coroutine::get_current();

    uv_timer_t timer;
    timer.data = co;
    uv_timer_init(uv_default_loop(), &timer);
    uv_timer_start(&timer, sleep_timeout, seconds, 0);
   
    co->yield();
    return 0;
}
```

其中

```cpp
Coroutine* co = Coroutine::get_current();
```

获取当前的协程。

```cpp
timer.data = co;
```

用来把当前协程保存在`timer`的数据字段里面，这样，当定时器`timerout`的时候，我们就可以通过`timer.data`取出这个协程。

```cpp
uv_timer_init(uv_default_loop(), &timer);
```

用来初始化我们的定时器节点。

```cpp
uv_timer_start(&timer, sleep_timeout, seconds, 0);
```

用来把这个定时器节点插入到整个定时器堆里面。这里，我们需要明确一点，`libuv`的这个定时器堆是一个最小堆，也就是说，堆顶的定时器节点的`timeout`越小。`uv_timer_start`函数里面会把定时器节点`timer`插入到整个定时器堆里面，并且会根据`timer`的`timeout`调整`timer`在定时器堆里面的位置。

```cpp
co->yield();
```

用来切换出当前协程，模拟出了协程自身阻塞的效果。

`OK`，我们现在来进行测试，重新编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

SCo::sleep(1);
```

执行：

```shell
~/codeDir/cppCode/study # php test.php 
Segmentation fault
~/codeDir/cppCode/study # 
```

完美报错。因为我们不在协程环境里面，所以我们不能够执行`SCo::sleep`。

我们修改我们的测试脚本：

```php
<?php

$t1 = time();

$cid = Sgo(function () {
    echo "before sleep" . PHP_EOL;
    // SCo::yield();
    SCo::sleep(1);
    echo "after sleep" . PHP_EOL;
});

echo "main co" . PHP_EOL;
```

然后执行我们的测试脚本：

```shell
~/codeDir/cppCode/study # php test.php 
before sleep
main co
~/codeDir/cppCode/study # 
```

发现没有阻塞`1s`并且里面退出了对吧。这很正常，因为当我们调用`SCo::sleep`的时候，就会把当前的协程切换出去，然后切换到我们的`'main coroutine'`了。而执行完了`main coroutine`之后，整个`PHP`进程就已经退出了，当然会立马退出了。





