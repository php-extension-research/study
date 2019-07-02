# 协程yield

上篇文章我们成功的创建出了`PHP`协程，这篇文章，我们来实现一下协程比较常用的两个接口：`yield`和`resume`。

我们先来实现一下`yield`。我们在文件`study_coroutine_util.cc`中进行实现。

我们需要定义一下这个接口的参数，因为我们并不需要给这个接口传递参数，所以我们只需要如下定义一个`PHP`的`void`参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_void, 0, 0, 0)
ZEND_END_ARG_INFO()
```

```cpp
PHP_METHOD(study_coroutine_util, yield)
{
    Coroutine* co = Coroutine::get_current();
    user_yield_coros[co->get_cid()] = co;
    co->yield();
    RETURN_TRUE;
}
```

然后，我们需要注册这个接口：

```cpp
static const zend_function_entry study_coroutine_util_methods[] =
{
    PHP_ME(study_coroutine_util, create, arginfo_study_coroutine_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(study_coroutine_util, yield, arginfo_study_coroutine_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

也就是说，这这方法是`public`和`static`的。

OK，我们来讲解一下这个接口的具体实现：

```cpp
Coroutine* co = Coroutine::get_current();
```

获取当前协程`*Study::Coroutine::current`。

```cpp
user_yield_coros[co->get_cid()] = co;
```

把获取的当前协程存入一个无序字典`user_yield_coros`里面。我们在文件`study_coroutine_util.cc`中进行定义：

```cpp
#include <unordered_map>
static std::unordered_map<long, Coroutine *> user_yield_coros;
```

```cpp
co->get_cid()
```

用于获取当前协程的id，我们直接在`Study::Coroutine`类里面定义它为`inline`的即可：

```cpp
public:
inline long get_cid()
{
  return cid;
}
```

最后，我们调用`Study::Coroutine`的`yield`接口把当前协程切换出去，换另一个协程执行即可。我们现在类`Study::Coroutine`中进行声明：

```cpp
public:
    void yield();
```

然后在`src/coroutine/coroutine.cc`中进行实现：

```cpp
void Coroutine::yield()
{
    current = origin;
    ctx.swap_out();
}
```

OK，我们现在实现完了`PHP`协程的`yield`。我们现在来进行测试。脚本如下：

```php
<?php

function task($n, $arg)
{
	echo "coroutine [$n]" . PHP_EOL;
	Study\Coroutine::yield();
	echo $arg . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 1, 'a');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 2, 'b');
echo "main coroutine" . PHP_EOL;

```

然后编译、安装扩展：

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
coroutine [1]
main coroutine
coroutine [2]
main coroutine
~/codeDir/cppCode/study # 
```

符合我们的预期。

[下一篇：协程resume](./《PHP扩展开发》-协程-协程resume.md)

