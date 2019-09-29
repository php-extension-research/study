# 修复一些bug（十一）

这里，修复一个`bug`，关于`PHP`栈的问题。

实际上，在我们的一个协程销毁的时候，`origin`协程的`PHP`栈并没有恢复。我们可以测试一下。在文件`study_coroutine.cc`的`study::PHPCoroutine::create_func`方法里面，释放`PHP`协程栈的地方：

```cpp
zend_vm_stack stack = EG(vm_stack);
php_printf("%p\n", stack);
efree(stack);
```

这里，我们打印了`PHP`栈的地址。然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

我们编写测试脚本来测试一下：

```php

<?php

study_event_init();

Sgo(function ()
{
    Sgo(function() {
        Sgo(function() {

        });
    });
});

study_event_wait();
```

这里简单的创建了三个协程。

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php

0x7fc757e81000
0x7fc757e81000
0x7fc757e81000
~/codeDir/cppCode/study #
```

我们发现，释放的协程栈都是同一个地址，这显然是不对的。实际上，如果大家去调试的话会发现，释放的一直是最后一个`PHP`栈的地址，这显然是不对的，这回造成内存泄漏。所以我们得去修复这个问题。

我们可以增加一个`on_close`方法，在`PHP`协程被销毁之前执行这个函数。

首先，在文件`include/coroutine.h`的`study::Coroutine`类里面声明：

```cpp
public:
    static void set_on_close(st_coro_on_swap_t func);
```

然后在这个类里面定义一个函数指针变量：

```cpp
protected:
    static st_coro_on_swap_t on_close;   /* before close */
```

然后，我们在`src/coroutine/coroutine.cc`里面初始化这个函数指针：

```cpp
st_coro_on_swap_t Coroutine::on_yield = nullptr;
st_coro_on_swap_t Coroutine::on_resume = nullptr;
st_coro_on_swap_t Coroutine::on_close = nullptr; // 新增的一行
```

然后，我们在每一个协程会被删除之前，调用这个`on_close`回调函数。首先是`study::Coroutine::run()`方法里面：

```cpp
if (ctx.is_end())
{
    on_close(task); // 新增的一行
    current = origin;
    coroutines.erase(cid);
    delete this;
}
```

然后是`study::Coroutine::resume()`方法里面：

```cpp
if (ctx.is_end())
{
    on_close(task); // 新增的一行
    current = origin;
    coroutines.erase(cid);
    delete this;
}
```

然后，我们在文件`src/coroutine/coroutine.cc`里面去实现`study::Coroutine::set_on_close`：

```cpp
void Coroutine::set_on_close(st_coro_on_swap_t func)
{
    on_close = func;
}
```

接着，我们需要在`PHPCoroutine::init()`方法里面去设置这个`on_close`函数指针：

```cpp
void PHPCoroutine::init()
{
    Coroutine::set_on_yield(on_yield);
    Coroutine::set_on_resume(on_resume);
    Coroutine::set_on_close(on_close);
}
```

然后，我们需要去实现`study::PHPCoroutine::on_close`的具体代码，实际上就是去销毁当前协程的`PHP`栈，以及恢复上一个协程的`PHP`栈。

现在类`study::PHPCoroutine`里面声明这个函数：

```cpp
protected:
    static void on_close(void *arg);
```

然后在文件`study_coroutine.cc`里面去实现这个方法：

```cpp
void PHPCoroutine::on_close(void *arg)
{
    php_coro_task *task = (php_coro_task *) arg;
    php_coro_task *origin_task = get_origin_task(task);
    zend_vm_stack stack = EG(vm_stack);
    php_printf("%p\n", stack);
    efree(stack);
    restore_task(origin_task);
}
```

这里，为了测试释放的协程栈地址，我们依旧打印`stack`的值。（测试完之后，小伙伴们自行删除这一行）

接着，我们删除在`study::PHPCoroutine::create_func`里面释放`PHP`协程栈的代码，因为我们删除`PHP`协程栈的代码都通过`on_close`回调函数来执行了：

```cpp
// 需要删除的代码
zend_vm_stack stack = EG(vm_stack);
php_printf("%p\n", stack);
efree(stack);
// 需要删除的代码
```

接着，重新编译、安装扩展：

```cpp
~/codeDir/cppCode/study # make clean ; make ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后，再次执行测试脚本：

```shell
~/codeDir/cppCode/study # php test.php

0x7fc78d681000
0x7fc78d67f000
0x7fc78d67d000
~/codeDir/cppCode/study #
```

我们发现，每次释放的协程栈地址都是不同的了。符合预期。
