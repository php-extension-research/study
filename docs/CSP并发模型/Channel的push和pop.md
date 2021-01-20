# Channel的push和pop

这篇文章，我们来实现扩展的`push`和`pop`操作。

在文件`study_coroutine_channel.cc`里面，定义`push`的参数：

```cpp
using study::coroutine::Channel;

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_push, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()
```

然后，在文件`study_coroutine_channel.h`里面引入头文件：

```cpp
#include "coroutine_channel.h"
```

然后，我们需要修改构造函数，保存我们底层的`Channel`对象：

```cpp
static PHP_METHOD(study_coro_channel, __construct)
{
    zval zchan;
    zend_long capacity = 1;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(capacity)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (capacity <= 0)
    {
        capacity = 1;
    }

    zend_update_property_long(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("capacity"), capacity);

    Channel *chan = new Channel(capacity);
    ZVAL_PTR(&zchan, chan);
    zend_update_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), &zchan);
}
```

然后注册一个属性`zchan`，用来保存`Channel`对象：

```cpp
void study_coro_channel_init()
{
    zval zchan;

    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property(study_coro_channel_ce_ptr, ZEND_STRL("zchan"), &zchan, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}
```

接着，可以去实现`push`接口了：

```cpp
static PHP_METHOD(study_coro_channel, push)
{
    zval *zchan;
    Channel *chan;
    zval *zdata;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(zdata)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    zchan = st_zend_read_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), 0);
    chan = (Channel *)Z_PTR_P(zchan);

    if (!chan->push(zdata, timeout))
    {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
```

代码很简单，接收`PHP`脚本传递过来的数据，然后`push`进数据队列里面。

然后注册这个方法：

```cpp
static const zend_function_entry study_coro_channel_methods[] =
{
    PHP_ME(study_coro_channel, __construct, arginfo_study_coro_channel_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_ME(study_coro_channel, push, arginfo_study_coro_channel_push, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

接下来，我们实现一下`pop`的操作。首先定义参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_pop, 0, 0, 0)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()
```

然后实现`pop`方法：

```cpp
static PHP_METHOD(study_coro_channel, pop)
{
    zval *zchan;
    Channel *chan;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    zchan = st_zend_read_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), 0);
    chan = (Channel *)Z_PTR_P(zchan);
    zval *zdata = (zval *)chan->pop(timeout);
    if (!zdata)
    {
        RETURN_FALSE;
    }
    RETVAL_ZVAL(zdata, 0, 0);
}
```

然后注册这个`pop`方法：

```cpp
static const zend_function_entry study_coro_channel_methods[] =
{
    PHP_ME(study_coro_channel, __construct, arginfo_study_coro_channel_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_ME(study_coro_channel, push, arginfo_study_coro_channel_push, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_channel, pop, arginfo_study_coro_channel_pop, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
```

`OK`，重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

编写测试脚本：

```php
<?php

study_event_init();

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan)
{
    $ret = $chan->pop();
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    $ret = $chan->push("hello world");
    var_dump($ret);
});

study_event_wait();
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php
string(11) "hello world"
bool(true)
^C
~/codeDir/cppCode/study #
```

得到了数据。但是，我们发现我们的程序一直不退出，这是不符合预期的。经过调试发现，问题出在了`st_event_wait`里面：

```cpp
while (StudyG.running > 0)
    {
        int n;
        uint64_t timeout;
        epoll_event *events;
```

我们发现，这里接收的`timeout`是一个无符号的，但是，如果我们没有定时，那么这个`timeout`返回值应该是`-1`。但是，如果这个是一个无符号的，那么就会把`-1`转化为一个非常大的正数。

我们优化一下这块代码：

```cpp
int st_event_wait()
{
    st_event_init();

    while (StudyG.running > 0)
    {
        int n;
        int64_t timeout;
        epoll_event *events;

        timeout = timer_manager.get_next_timeout();
        events = StudyG.poll->events;

        /**
         * If there are no timers and events
         */
        if (timeout < 0 && StudyG.poll->event_num == 0)
        {
            StudyG.running = 0;
            break;
        }

        /**
         * Handle timer tasks
         */
        timer_manager.run_timers();

        n = epoll_wait(StudyG.poll->epollfd, events, StudyG.poll->ncap, timeout);

        for (int i = 0; i < n; i++) {
            int fd;
            int id;
            struct epoll_event *p = &events[i];
            uint64_t u64 = p->data.u64;
            Coroutine *co;

            fromuint64(u64, &fd, &id);
            co = Coroutine::get_by_cid(id);
            co->resume();
        }
    }

    st_event_free();

    return 0;
}
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

重新执行脚本：

```shell
~/codeDir/cppCode/study # php test.php
string(11) "hello world"
bool(true)
~/codeDir/cppCode/study #
```

此时，进程不会无限阻塞了。这是先`pop`再`push`。

我们换一个测试脚本：

```php
<?php

study_event_init();

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan)
{
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    $ret = $chan->pop();
    var_dump($ret);
});

study_event_wait();
```

我们先`push`，然后再`pop`。看看有啥问题：

```shell
~/codeDir/cppCode/study # php test.php
bool(true)
bool(true)
~/codeDir/cppCode/study #
```

我们发现，取不出传入的字符串`hello world`。

这是为什么呢？我们来修改一下测试脚本，多打印一些调试信息：

```php
<?php

study_event_init();

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan)
{
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});

study_event_wait();
```

然后重新执行脚本：

```shell
~/codeDir/cppCode/study # php test.php
string(10) "push start"
bool(true)
string(9) "pop start"
string(9) "pop start"
~/codeDir/cppCode/study #
```

我们发现，数据完全错乱了。这是为什么呢？

因为我们从`PHP`脚本传入到扩展的时候，字符串出问题了，被销毁了，所以会出问题。我们修改`push`接口：

```cpp
Z_TRY_ADDREF_P(zdata); // 增加的地方
zdata = st_zval_dup(zdata); // 增加的地方

if (!chan->push(zdata, timeout))
{
    Z_TRY_DELREF_P(zdata); // 增加的地方
    efree(zdata); // 增加的地方
    RETURN_FALSE;
}
```

然后，在文件`php_study.h`里面实现`st_zval_dup`函数：

```cpp
inline zval* st_malloc_zval()
{
    return (zval *) emalloc(sizeof(zval));
}

inline zval* st_zval_dup(zval *val)
{
    zval *dup = st_malloc_zval();
    memcpy(dup, val, sizeof(zval));
    return dup;
}
```

对应的，`pop`接口需要释放掉拷贝出来的内存：

```cpp
RETVAL_ZVAL(zdata, 0, 0);
efree(zdata); // 增加的地方
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

然后编写测试脚本：

```php
<?php

study_event_init();

$chan = new Study\Coroutine\Channel();

Sgo(function () use ($chan)
{
    var_dump("push start");
    $ret = $chan->push("hello world");
    var_dump($ret);
});

Sgo(function () use ($chan)
{
    var_dump("pop start");
    $ret = $chan->pop();
    var_dump($ret);
});

study_event_wait();
```

执行测试脚本：

```shell
~/codeDir/cppCode/study # php test.php
string(10) "push start"
bool(true)
string(9) "pop start"
string(11) "hello world"
~/codeDir/cppCode/study #
```

我们发现成功的获取到了`push`进去的数据。

这里需要拷贝一份的原因是：第一个协程`push`进`Channel`的数据是一个字符串，`PHP`扩展解析的时候，没有拷贝出一个副本，只是增加了引用计数。所以，`push`进去的数据始终存在于第一个协程的栈里面。当第一个协程`push`完数据之后，它的协程栈立马就被销毁了。等到第二个协程使用这个数据的时候，这个数据已经不在了，所以这里需要拷贝一份出来。至于这里为什么需要增加引用计数，是因为防止其他协程`unset`了这个数据，导致其他的协程用不来。更详细的解释参考我的另一篇文章：
[《PHP扩展开发--引用计数的使用》](https://huanghantao.github.io/2019/09/24/%E3%80%8APHP%E6%89%A9%E5%B1%95%E5%BC%80%E5%8F%91-%E5%BC%95%E7%94%A8%E8%AE%A1%E6%95%B0%E7%9A%84%E4%BD%BF%E7%94%A8%E3%80%8B/)

那么为什么我们的第一个测试脚本先`pop`再`push`就可以取出数据呢？因为当第一个协程`pop`的时候，`Channel`没有数据，被`yield`出去了。然后第二个协程`push`数据，`push`完之后，`resume`第一个协程，此时第二个协程的栈还没有被销毁，因此`push`进去的字符串还可以使用。

[下一篇：修复一些bug（十）](./《PHP扩展开发》-协程-修复一些bug（十）.md)
