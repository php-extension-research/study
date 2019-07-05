# 协程短名（二）

我们还是觉得创建协程的方法名太长了，我们还想再缩短一下，变成这样：

```php
sco::create($callable);
```

OK，首先，我们在`study_coroutine_util_methods`里面新增一行：

```cpp
PHP_MALIAS(study_coroutine_util, sco, create, arginfo_study_coroutine_defer, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
```

这个宏展开如下：

```cpp
#define PHP_MALIAS      ZEND_MALIAS

#define ZEND_MALIAS(classname, name, alias, arg_info, flags) \
                                                    ZEND_FENTRY(name, ZEND_MN(classname##_##alias), arg_info, flags)
```

所以，我们不要这么写：

```cpp
PHP_MALIAS(study_coroutine_util, create, sco, arginfo_study_coroutine_defer, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
```

否则宏展开将会得到如下内容：

```cpp
ZEND_FENTRY(name, ZEND_MN(study_coroutine_util##_##sco), arg_info, flags)
```

很显然，`zim_study_coroutine_util_sco`是没有定义的，所以会报错。这一点大家要注意了。别写反了。

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

$cid1 = sco::create('task');
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





