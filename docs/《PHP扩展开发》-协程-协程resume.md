# 协程resume

这篇文章，我们来实现一下我们协程的`resume`接口。我们先定义一下我们的接口参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_resume, 0, 0, 1)
    ZEND_ARG_INFO(0, cid)
ZEND_END_ARG_INFO()
```

说明我们打算接收一个协程的id，然后`resume`它。

接着，我们来实现一下`resume`接口：

```cpp
PHP_METHOD(study_coroutine_util, resume)
{
    zend_long cid = 0;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(cid)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    auto coroutine_iterator = user_yield_coros.find(cid);

    Coroutine* co = coroutine_iterator->second;
    user_yield_coros.erase(cid);
    co->resume();
    RETURN_TRUE;
}
```

然后，我们注册这个接口：

```cpp
static const zend_function_entry study_coroutine_util_methods[] =
{
    PHP_ME(study_coroutine_util, create, arginfo_study_coroutine_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(study_coroutine_util, yield, arginfo_study_coroutine_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(study_coroutine_util, resume, arginfo_study_coroutine_resume, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

我们来分析一下这个接口的实现。

其中

```cpp
ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(cid)
ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
```

不多说了，解析传递个`resume`接口的参数，是一个协程id。

```cpp
auto coroutine_iterator = user_yield_coros.find(cid);

Coroutine* co = coroutine_iterator->second;
user_yield_coros.erase(cid);
```

在我们的无序字典里面通过协程id来查找到我们的协程。并且把它从这个无序字典里面删除。

```cpp
co->resume();
```

`resume`这个被指定的协程。

OK，我们来实现一下`Study::Coroutine`的`yield`。首先，我们在`Study::Coroutine`这个类里面进行声明：

```cpp
public:
    void resume();
```

然后我们在`src/coroutine/coroutine.cc`这个类里面去实现它：

```cpp
void Coroutine::resume()
{
    origin = current;
    current = this;
    ctx.swap_in();
}
```

其中：

```cpp
origin = current;
current = this;
```

是我们熟悉的代码，用来更替当前协程。

```cpp
ctx.swap_in();
```

让当前协程的上下文载入。

OK，我们来编写对应的测试脚本：

```php
<?php

function task($n, $arg)
{
	echo "coroutine [$n] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [$n] be resumed" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 1, 'a');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 2, 'b');
echo "main coroutine" . PHP_EOL;

Study\Coroutine::resume(1);
echo "main coroutine" . PHP_EOL;
Study\Coroutine::resume(2);
echo "main coroutine" . PHP_EOL;
```

我们编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

然后执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] be resumed
main coroutine
coroutine [2] be resumed
main coroutine
~/codeDir/cppCode/study # 
```

符合我们的预期。

[下一篇：协程getCid](./《PHP扩展开发》-协程-协程getCid.md)







