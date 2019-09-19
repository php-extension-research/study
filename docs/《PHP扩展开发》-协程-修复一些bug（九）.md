# 修复一些bug（九）

我们在方法`Socket::wait_event`里面，当事件被处理完的时候，没有去删除这个事件，这显然是不符合逻辑的。我们做出如下修改：

```cpp
co->yield();

// 以下是增加的代码
if (epoll_ctl(StudyG.poll->epollfd, EPOLL_CTL_DEL, sockfd, NULL) < 0)
{
    stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    return false;
}
```

编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

符合预期。

然后，我们把所有的`malloc`以及`free`替换成`new`和`delete`。

首先是修改`init_stPoll`这个函数：

```cpp
int init_stPoll()
{
    try
    {
        StudyG.poll = new stPoll_t();
    }
    catch(const std::exception& e)
    {
        stError("%s", e.what());
    }

    StudyG.poll->epollfd = epoll_create(256);
    if (StudyG.poll->epollfd  < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        delete StudyG.poll;
        StudyG.poll = nullptr;
        return -1;
    }

    StudyG.poll->ncap = 16;
    try
    {
        StudyG.poll->events = new epoll_event[StudyG.poll->ncap](); // zero initialized
    }
    catch(const std::bad_alloc& e)
    {
        stError("%s", e.what());
    }
    StudyG.poll->event_num = 0;

    return 0;
}
```

这里，我们用`try catch`来捕获`new`抛出的异常，例如内存不够的时候，我们是可以获取这个信息的。

然后是修改函数`free_stPoll`：

```cpp
int free_stPoll()
{
    if (close(StudyG.poll->epollfd) < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
    }
    delete[] StudyG.poll->events;
    StudyG.poll->events = nullptr;
    delete StudyG.poll;
    StudyG.poll = nullptr;
    return 0;
}
```

然后是`st_event_free`里面，我们忘记修改`running`的值了：

```cpp
int st_event_free()
{
    StudyG.running = 0; // 增加的代码
    free_stPoll();
    return 0;
}
```

然后是`Context::Context`这个方法：

```cpp
Context::Context(size_t stack_size, coroutine_func_t fn, void* private_data) :
        fn_(fn), stack_size_(stack_size), private_data_(private_data)
{
    swap_ctx_ = nullptr;

    try
    {
        stack_ = new char[stack_size_];
    }
    catch(const std::bad_alloc& e)
    {
        stError("%s", e.what());
    }

    void* sp = (void*) ((char*) stack_ + stack_size_);
    ctx_ = make_fcontext(sp, stack_size_, (void (*)(intptr_t))&context_func);
}
```

然后是方法`Context::~Context()`：

```cpp
Context::~Context()
{
    if (swap_ctx_)
    {
        delete[] stack_;
        stack_ = nullptr;
    }
}
```

然后是方法`Socket::init_read_buffer`：

```cpp
int Socket::init_read_buffer()
{
    if (!read_buffer)
    {
        try
        {
            read_buffer = new char[65536];
        }
        catch(const std::bad_alloc& e)
        {
            stError("%s", e.what());
        }

        read_buffer_len = 65536;
    }

    return 0;
}
```

然后是方法`Socket::init_write_buffer`：

```cpp
int Socket::init_write_buffer()
{
    if (!write_buffer)
    {
        try
        {
            write_buffer = new char[65536];
        }
        catch(const std::bad_alloc& e)
        {
            stError("%s", e.what());
        }

        write_buffer_len = 65536;
    }

    return 0;
}
```

`OK`，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后写一个简单的脚本测试一下：

```php
<?php

study_event_init();

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});

Sgo(function () {
    var_dump(Sco::getCid());
    Sco::sleep(1);
    var_dump(Sco::getCid());
});
study_event_wait();
```

执行结果：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(2)
int(3)
int(1)
int(2)
int(3)
~/codeDir/cppCode/study #
```

符合预期。

[下一篇：重构定时器（一）](./《PHP扩展开发》-协程-重构定时器（一）.md)
