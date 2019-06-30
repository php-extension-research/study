# 协程创建（七）

我们在上篇文章，成功的保存了主协程的上下文信息，现在，我们就需要为我们的任务函数创建协程了。

我们在`PHPCoroutine::create`中写入：

```cpp
long PHPCoroutine::create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv)
{
    php_coro_args php_coro_args;
    php_coro_args.fci_cache = fci_cache;
    php_coro_args.argv = argv;
    php_coro_args.argc = argc;
    save_task(get_task());

    return Coroutine::create(create_func, (void*) &php_coro_args);
}
```

其中，`PHPCoroutine::create_func`是用来创建协程任务函数的。它是：

```cpp
typedef void(* coroutine_func_t)(void *) 
```

类型的函数指针。

而`php_coro_args`则是传递给`create_func`的参数。

OK，我们现在来实现一下`PHPCoroutine::create_func`。我们先在`Study::PHPCoroutine`类中声明一下这个方法：

```cpp
protected:
    static void create_func(void *arg);
```

然后，我们在文件`study_coroutine.cc`中来实现这个函数：

```cpp

```

