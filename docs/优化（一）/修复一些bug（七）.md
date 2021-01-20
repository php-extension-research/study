# 修复一些bug（七）

这篇文章，我们来修复一个非常严重的`bug`。我们来看看，这是测试脚本：

```php
<?php

while (true) {
    Sgo(function () {
        $cid = Sco::getCid();
        var_dump($cid);
    });
}
```

运行测试脚本：

```shell
int(16223)
int(16224)

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 8192 bytes) in /root/codeDir/cppCode/study/test.php on line 7
```

最终你会看到，我们最多创建了`16224`个协程，这是在我的机器上面测到的数据。我们消耗了`134M`的内存，这显然不是我们希望看到的对吧。因为我们的协程被创建完之后就立即死亡了，不存在常驻内存，所以，一定是我们没有释放某些内存。我们很容易可以想到，协程运行需要一个协程栈，而且这个栈我们默认给的是`2M`，所以，肯定是它没有释放掉。我们来看看启动协程的代码：

```cpp
long run()
{
    long cid = this->cid;
    origin = current;
    current = this;
    ctx.swap_in();
    if (ctx.is_end())
    {
        current = origin;
        coroutines.erase(cid);
        delete this;
    }
    return cid;
}
```

我们发现，在协程结束的时候，我们只是释放掉了`this`，也就是`Coroutine`对象自己。但是没有释放掉对应的栈内存。我们需要修复这个问题。

我们来看看我们的栈是在哪里分配的：

```cpp
Context::Context(size_t stack_size, coroutine_func_t fn, void* private_data) :
        fn_(fn), stack_size_(stack_size), private_data_(private_data)
{
    swap_ctx_ = nullptr;

    stack_ = (char*) malloc(stack_size_);

    void* sp = (void*) ((char*) stack_ + stack_size_);
```

在`Context`构造函数里面分配的。所以，很明显，我们可以在它的析构函数里面释放掉这片内存。我们在文件`include/context.h`里面声明析构函数：

```cpp
public:
    ~Context();
```

然后在文件`src/coroutine/context.cc`里面实现：

```cpp
Context::~Context()
{
    if (swap_ctx_)
    {
        free(stack_);
        stack_ = NULL;
    }
}
```

我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后，我们再次运行脚本：

```shell
~/codeDir/cppCode/study # php test.php
int(16223)
int(16224)

Fatal error: Allowed memory size of 134217728 bytes exhausted (tried to allocate 8192 bytes) in /root/codeDir/cppCode/study/test.php on line 7
~/codeDir/cppCode/study #
```

我们发现，还是会有内存泄漏。因为我们只是释放了`C`栈，没有释放`PHP`栈，我们去释放一下。在`PHP`协程入口函数：

```cpp
static void study::PHPCoroutine::create_func(void *arg)
```

里面。我们在这个方法末尾加上释放`PHP`栈的代码：

```cpp
zend_vm_stack stack = EG(vm_stack); // 增加的代码
efree(stack); // 增加的代码

zval_ptr_dtor(retval);
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后运行测试脚本：

```shell
~/codeDir/cppCode/study # php test.php

```

我们发现，我们跑了`413986`个协程都没有造成内存泄漏（你可以通过`top`命令来查看内存占用情况）：

```shell
int(413983)
int(413984)
int(413985)
int(413986)
^C
~/codeDir/cppCode/study #
```

(其实可以创建无数个协程，只是我杀死了进程而已)

教程写到这里，其实很多问题我都是知道的，为什么留到现在才讲解呢？因为学习是一个循序渐进的过程，如果我之前就把代码写完整了，那么就会少了很多分析问题的过程，而分析的过程，实际上才是最有价值的。

[下一篇：错误使用协程库导致的Bug（一）](./《PHP扩展开发》-协程-错误使用协程库导致的Bug（一）.md)
