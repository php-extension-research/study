# 协程defer

这篇文章，我们来实现一下协程的：

```php
Study\Coroutine::defer($callable)
```

首先，我们来定义一下接口的参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_defer, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, func, 0)
ZEND_END_ARG_INFO()
```

然后我们注册这个接口：

```cpp
static const zend_function_entry study_coroutine_util_methods[] =
{
    // 省略了其他的注册
    PHP_ME(study_coroutine_util, defer, arginfo_study_coroutine_defer, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

我们规定，必须在协程环境下才可以调用这个接口。因为，`$callable`这个函数是在协程结束的时候执行的。

OK，我们来实现接口：

```cpp
PHP_METHOD(study_coroutine_util, defer)
{
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    php_study_fci_fcc *defer_fci_fcc;

    defer_fci_fcc = (php_study_fci_fcc *)emalloc(sizeof(php_study_fci_fcc));

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    defer_fci_fcc->fci = fci;
    defer_fci_fcc->fcc = fcc;

    PHPCoroutine::defer(defer_fci_fcc);
}
```

其中

`php_study_fci_fcc`是一个包含了`zend_fcall_info`和`zend_fcall_info_cache`的结构体。我们在`study_coroutine.h`文件中进行定义：

```cpp
struct php_study_fci_fcc
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
};
```

我们很容易想到，在`PHPCoroutine::defer`里面肯定会用到我们之前讲解的**如何在扩展中调用用户空间传递的函数**这个知识点（我们后面会讲解）。

OK，我们来实现一下`PHPCoroutine::defer`。我们先在`study_coroutine.h`这个文件中的`study::PHPCoroutine`类里面声明：

```cpp
public:
    static void defer(php_study_fci_fcc *defer_fci_fcc);
```

又因为我们的`$callable`是在协程即将死亡的时候执行的，所以肯定是在`PHP`协程入口这个函数里面执行`$callable`，因此，我们需要给`php_coro_args`这个结构增加一个属性来传递我们的`fci`和`fcc`。这里，我们使用栈来保存`php_study_fci_fcc *`（记得`include <stack>`一下）。

```cpp
#include <stack>

struct php_coro_task
{
    // 省略其他的成员
    std::stack<php_study_fci_fcc *> *defer_tasks;
};
```

我们新增了`defer_tasks`成员。为什么使用栈呢？因为先用`defer`注册的的函数，反而后被执行，这符合栈的思想。

OK，我们来实现一下`PHPCoroutine::defer`，在`study_coroutine.cc`这个文件里面：

```cpp
void PHPCoroutine::defer(php_study_fci_fcc *defer_fci_fcc)
{
    php_coro_task *task = (php_coro_task *)get_task();
    if (task->defer_tasks == nullptr)
    {
        task->defer_tasks = new std::stack<php_study_fci_fcc *>;
    }
    task->defer_tasks->push(defer_fci_fcc);
}
```

我们获取当前协程的`php_coro_task`，然后把这个`defer_fci_fcc`给存进去。

OK，现在我们需要做的就是在协程死亡的时候调用我们传递的用户函数即可。

在`PHP`的协程入口函数`PHPCoroutine::create_func`中完成这个任务。我们在如下代码：

```cpp
task->co = Coroutine::get_current();
task->co->set_task((void *) task);
```

的后面紧接着增加如下代码：

```cpp
task->defer_tasks = nullptr;
```

因为初始化的`defer_tasks`不一定就是`nullptr`。如果`defer_tasks`的初始化值不为`nullptr`，那么会影响如下函数：

```cpp
void PHPCoroutine::defer(php_study_fci_fcc *defer_fci_fcc)
{
    php_coro_task *task = (php_coro_task *)get_task();
    if (task->defer_tasks == nullptr)
    {
        task->defer_tasks = new std::stack<php_study_fci_fcc *>;
    }
    task->defer_tasks->push(defer_fci_fcc);
}
```

因为`defer_tasks`的初始值不是`nullptr`，那么就不会创建栈，会导致后面的`push`操作失败。这里为了防止初始化的`defer_tasks`不为`nullptr`，我们手动设置为`nullptr`。

OK，我们继续。

在如下代码：

```cpp
if (func->type == ZEND_USER_FUNCTION)
{
    ZVAL_UNDEF(retval);
    EG(current_execute_data) = NULL;
    zend_init_func_execute_data(call, &func->op_array, retval);
    zend_execute_ex(EG(current_execute_data));
}
```

的后面紧接着增加如下代码：

```cpp
if (defer_tasks) {
    php_study_fci_fcc *defer_fci_fcc;
    zval result;
    while(!defer_tasks->empty())
    {
        defer_fci_fcc = defer_tasks->top();
        defer_tasks->pop();
        defer_fci_fcc->fci.retval = &result;

        if (zend_call_function(&defer_fci_fcc->fci, &defer_fci_fcc->fcc) != SUCCESS)
        {
            php_error_docref(NULL, E_WARNING, "defer execute error");
            return;
        }
        efree(defer_fci_fcc);
    }
    delete defer_tasks;
    task->defer_tasks = nullptr;
}
```

代码很简单，就是一直把栈里面需要调用的用户函数一直弹出栈，然后再执行。

然后编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

然后编写如下测试脚本：

```php
<?php

function deferFunc()
{
    echo "in defer method" . PHP_EOL;
}

function task()
{
    echo "task coroutine start" . PHP_EOL;
    Study\Coroutine::defer('deferFunc');
    echo "task coroutine end" . PHP_EOL;
}

$cid1 = Study\Coroutine::create('task');
```

然后执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
task coroutine start
task coroutine end
in defer method
~/codeDir/cppCode/study # 
```

我们再注册几个`defer`：

```php
<?php

function deferFunc1()
{
    echo "in defer deferFunc1" . PHP_EOL;
}

function deferFunc2()
{
    echo "in defer deferFunc2" . PHP_EOL;
}

function deferFunc3()
{
    echo "in defer deferFunc3" . PHP_EOL;
}

function task()
{
    echo "task coroutine start" . PHP_EOL;
    Study\Coroutine::defer('deferFunc1');
    Study\Coroutine::defer('deferFunc2');
    Study\Coroutine::defer('deferFunc3');
    echo "task coroutine end" . PHP_EOL;
}

$cid1 = Study\Coroutine::create('task');
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
task coroutine start
task coroutine end
in defer deferFunc3
in defer deferFunc2
in defer deferFunc1
~/codeDir/cppCode/study # 
```

OK，符合预期。

[下一篇：协程短名（一）](./《PHP扩展开发》-协程-协程短名（一）.md)

