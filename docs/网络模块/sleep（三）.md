# sleep（三）

在上一篇文章中，我们成功的在`PHP`协程调用`sleep`接口的时候，切换出了自己的上下文。但是，我们并没有在`1s`后`'唤醒'`这个协程。这篇文章，我们开始来实现这个唤醒的功能。

在上一篇文章中，我们知道，一旦协程调用`sleep`接口切换出了上下文之后，由于`main coroutine`的退出，导致了整个进程的退出。所以，为了不让这个进程退出，我们显然需要一个无限循环对吧。我们把这个无限循环封装成一个协程调度器。我们也会暴露出这个协程调度器作为`PHP`脚本调用的接口。

我们这个协程调度器不需要接受任何参数，所以我们不需要单独为这个接口定义参数。我们直接定义接口就行了：

```cpp
PHP_METHOD(study_coroutine_util, scheduler)
{
    if (PHPCoroutine::scheduler() < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
```

其中，`PHPCoroutine::scheduler`是调度器，我们需要在`PHPCoroutine`类里面来声明一下：

```cpp
public:
    static int scheduler();
```

然后在`study_coroutine.cc`里面来实现它：

```cpp
int PHPCoroutine::scheduler()
{
    while (1)
    {
    }
}
```

目前，我们的`PHPCoroutine::scheduler`还只是一个无限循环，我们后面会去实现它。

然后，我们需要注册这个接口：

```cpp
static const zend_function_entry study_coroutine_util_methods[] =
{
    // 省略其他的注册方法
        PHP_ME(study_coroutine_util, scheduler, arginfo_study_coroutine_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

然后，重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

编写我们的测试脚本：

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

执行：

```shell
~/codeDir/cppCode/study # php test.php 
before sleep
main co
^C
~/codeDir/cppCode/study # 
```

我们发现，此时我们的进程不会退出了。但是，`1s`后协程并没有被唤醒。因为我们还没有去实现`PHPCoroutine::scheduler`的具体逻辑。

到了这里，可能就有小伙伴疑惑了。`Swoole`并没有`scheduler()`这样的方法呀，多么的方便啊。其实是这样的，`Swoole`的调度器功能实际上都被封装在了`Reactor`里面了。如果我们去实现`Reactor`就需要一些篇幅了，所以我并不打算把`Reactor`加入到文章里面。但是，如果有小伙伴们有需要，以后我可以把这里的`scheduler`给重构掉，尽可能的和`Swoole`的思路保持一致。

[下一篇：sleep（四）](./《PHP扩展开发》-协程-sleep（四）.md)





