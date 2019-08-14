# 协程短名（二）

我们还是觉得创建协程的方法名太长了，我们还想再缩短一下，变成这样：

```php
sgo($callable);
```

OK，首先，我们把：

```cpp
PHP_METHOD(study_coroutine_util, create)
```

修改为：

```cpp
PHP_FUNCTION(study_coroutine_create)
```

并且删除我们之前在`study_coroutine_util.cc`文件中声明的：

```cpp
static PHP_METHOD(study_coroutine_util, create);
```

也就是说，现在我们原来的方法改成了函数。

然后，我们在`study_coroutine_util_methods`中新增：

```cpp
ZEND_FENTRY(create, ZEND_FN(study_coroutine_create), arginfo_study_coroutine_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC) // ZEND_FENTRY这行是新增的
```

并且删除：

```cpp
PHP_ME(study_coroutine_util, create, arginfo_study_coroutine_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
```

然后，我们在`study.cc`中声明这个函数，以及参数：：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_create, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, func, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(study_coroutine_create);
```

然后在`study_functions`里面注册这个函数：

```cpp
const zend_function_entry study_functions[] = {
    PHP_FE(study_coroutine_create, arginfo_study_coroutine_create)
    PHP_FALIAS(sgo, study_coroutine_create, arginfo_study_coroutine_create)
    PHP_FE_END
};
```

`PHP_FALIAS`的作用就是用来取别名的，这里换成了我们需要的短名`sgo`。

OK，我们编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

测试脚本：

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
    SCo::defer('deferFunc1');
    SCo::defer('deferFunc2');
    SCo::defer('deferFunc3');
    echo "task coroutine end" . PHP_EOL;
}

$cid1 = sgo('task');
```

执行：

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

[下一篇：引入libuv](./《PHP扩展开发》-协程-引入libuv.md)
