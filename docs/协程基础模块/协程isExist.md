# 协程isExist

这篇文章，我们实现一个判断某个协程是否存在的接口：

```php
Study\Coroutine::isExist(long $cid): bool
```

我们先定义方法参数，在`study_coroutine_util.cc`文件中进行定义：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_isExist, 0, 0, 1)
    ZEND_ARG_INFO(0, cid)
ZEND_END_ARG_INFO()
```

方法接收一个协程id。

然后实现这个接口：

```cpp
PHP_METHOD(study_coroutine_util, isExist)
{
    zend_long cid = 0;
    bool is_exist;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(cid)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    auto coroutine_iterator = Coroutine::coroutines.find(cid);
    is_exist = (coroutine_iterator != Coroutine::coroutines.end());
    RETURN_BOOL(is_exist);
}
```

我们直接在`Coroutine::coroutines`中查找这个协程是否存在，如果找到，则返回`true`；否则返回`false`。

因为上一章我们修复了一个协程死亡后，没有从`Coroutine::coroutines`中删除的bug，所以，我们的`Study\Coroutine::isExist`可以正常使用了。

我们编写测试脚本：

```php
<?php

function task($arg)
{
	$cid = Study\Coroutine::getCid();
	echo "coroutine [{$cid}] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [{$cid}] is resumed" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
$cid1 = Study\Coroutine::create('task', 'a');
echo "main coroutine" . PHP_EOL;
$cid2 = Study\Coroutine::create('task', 'b');
echo "main coroutine" . PHP_EOL;

if (Study\Coroutine::isExist($cid1))
{
	echo "coroutine [{$cid1}] is existent\n";
}
else
{
	echo "coroutine [{$cid1}] is non-existent\n";
}

if (Study\Coroutine::isExist($cid2))
{
	echo "coroutine [{$cid2}] is existent\n";
}
else
{
	echo "coroutine [{$cid2}] is non-existent\n";
}

Study\Coroutine::resume($cid1);
echo "main coroutine" . PHP_EOL;
if (Study\Coroutine::isExist($cid1))
{
	echo "coroutine [{$cid1}] is existent\n";
}
else
{
	echo "coroutine [{$cid1}] is non-existent\n";
}

if (Study\Coroutine::isExist($cid2))
{
	echo "coroutine [{$cid2}] is existent\n";
}
else
{
	echo "coroutine [{$cid2}] is non-existent\n";
}
Study\Coroutine::resume($cid2);
echo "main coroutine" . PHP_EOL;
if (Study\Coroutine::isExist($cid1))
{
	echo "coroutine [{$cid1}] is existent\n";
}
else
{
	echo "coroutine [{$cid1}] is non-existent\n";
}

if (Study\Coroutine::isExist($cid2))
{
	echo "coroutine [{$cid2}] is existent\n";
}
else
{
	echo "coroutine [{$cid2}] is non-existent\n";
}
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
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] is existent
coroutine [2] is existent
coroutine [1] is resumed
main coroutine
coroutine [1] is non-existent
coroutine [2] is existent
coroutine [2] is resumed
main coroutine
coroutine [1] is non-existent
coroutine [2] is non-existent
~/codeDir/cppCode/study # 
```

符合预期。

[下一篇：修复一些bug（三）](./《PHP扩展开发》-协程-修复一些bug（三）.md)











