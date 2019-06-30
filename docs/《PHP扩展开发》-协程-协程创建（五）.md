# 协程创建（五）

上篇文章，我们成功的在创建协程的接口里面调用了用户空间传递过来的函数。接下来，我们来为这个任务函数创建一个协程，也就是要实现我们的函数：

```cpp
Study::PHPCoroutine::create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv);
```

我们先为协程的执行定义一个协程参数结构体，在文件`study_coroutine.h`中定义：

```cpp
struct php_coro_args
{
    zend_fcall_info_cache *fci_cache;
    zval *argv;
    uint32_t argc;
};
```

用来保存我们传递给`Study::PHPCoroutine::create`的一系列参数。

我们在`study_coroutine.cc`文件中实现`Study::PHPCoroutine::create`：

```cpp
#include "study_coroutine.h"

using Study::PHPCoroutine;

long PHPCoroutine::create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv)
{
    php_coro_args php_coro_args;
    php_coro_args.fci_cache = fci_cache;
    php_coro_args.argv = argv;
    php_coro_args.argc = argc;
}
```

然后，我们需要保存当前的`PHP`栈。我们通过函数`void PHPCoroutine::save_task(php_coro_task * task)`来实现这个功能。

我们先在`study_coroutine.h`的`Study::PHPCoroutine`这个类里面声明它为`protected`，因为我们不希望它被类外部调用：

```cpp
protected:
    static void save_task(php_coro_task *task);
```

然后，我们定义一下需要保存的协程状态信息结构体`php_coro_task`，我们在`study_coroutine.h`中定义：

```cpp
// save the coroutine stack info
struct php_coro_task
{
    zval *vm_stack_top; // coroutine stack top
    zval *vm_stack_end; // coroutine stack end
    zend_vm_stack vm_stack; // current coroutine stack pointer
    size_t vm_stack_page_size;
    zend_execute_data *execute_data; // current coroutine stack frame
};
```

其中：

`vm_stack_top`是协程栈栈顶。

`vm_stack_end`是协程栈栈底。

`vm_stack`是协程栈指针。

`vm_stack_page_size`是协程栈页大小。

`execute_data`是当前协程栈的栈帧。

OK，我们现在来实现一下`PHPCoroutine::save_task`，在文件`study_coroutine.cc`中实现：

```cpp
void PHPCoroutine::save_task(php_coro_task *task)
{
    save_vm_stack(task);
}
```

然后，我们编写保存当前协程栈内容的`void PHPCoroutine::save_vm_stack(php_coro_task * task)`函数。

我们先在`study_coroutine.h`的`Study::PHPCoroutine`这个类里面声明它为`protected`，因为我们不希望它被类外部调用：

```cpp
protected:
    static void save_task(php_coro_task *task);
    static void save_vm_stack(php_coro_task *task);
```

然后，我们在`study_coroutine.cc`文件里面来实现这个函数：

```cpp
void PHPCoroutine::save_vm_stack(php_coro_task *task)
{
    task->vm_stack_top = EG(vm_stack_top);
    task->vm_stack_end = EG(vm_stack_end);
    task->vm_stack = EG(vm_stack);
    task->vm_stack_page_size = EG(vm_stack_page_size);
    task->execute_data = EG(current_execute_data);
}
```

接下来，我们就需要去获取当前的协程栈信息，我们通过函数`php_coro_task* PHPCoroutine::get_task()`来实现。

我们先在`study_coroutine.h`的`Study::PHPCoroutine`这个类里面声明它为`protected`，因为我们不希望它被类外部调用：

```cpp
protected:
    static void save_task(php_coro_task *task);
    static void save_vm_stack(php_coro_task *task);
    static php_coro_task* get_task();
```

然后，我们在`study_coroutine.cc`文件里面来实现这个函数：

```cpp
php_coro_task* PHPCoroutine::get_task()
{
    return nullptr;
}
```

（限于文章篇幅原因，我打算开其他文章来讲解`get_task`的实现。）

最后，我们在创建协程的接口里面调用`Study::PHPCoroutine::create`即可：

```cpp
using Study::PHPCoroutine;

PHP_METHOD(study_coroutine_util, create)
{
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zval result;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_FUNC(fci, fcc)
        Z_PARAM_VARIADIC('*', fci.params, fci.param_count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    PHPCoroutine::create(&fcc, fci.param_count, fci.params);
}
```

我们来编译安装一下，看看是否可以通过编译：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

编译成功。

我们现在来测试一下脚本：

```php
<?php

Study\Coroutine::create(function ($a, $b){
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}, 'a', 'b');
```

```shell
~/codeDir/cppCode/study # php test.php 
Segmentation fault
~/codeDir/cppCode/study # 
```

很正常，因为我们在`PHPCoroutine::get_task`中返回的是一个空指针，所以在执行`PHPCoroutine::save_vm_stack`的时候当然会报`Segmentation fault`的错误。



