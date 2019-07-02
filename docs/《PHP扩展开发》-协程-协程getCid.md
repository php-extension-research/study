# 协程getCid

在上篇文章中，我们写了如下测试脚本：

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

我们的协程任务函数`task`有一个参数`$n`，它是用来接收协程id的。很显然，这是不友好的。我们在开发的时候肯定不可能这样做对吧。所以我们需要提供一个接口来获取当前协程的id。

我们在`study_coroutine_util.cc`中进行实现：

```cpp
PHP_METHOD(study_coroutine_util, getCid)
{
    Coroutine* co = Coroutine::get_current();
    RETURN_LONG(co->get_cid());
}
```

然后注册这个函数：

```cpp
PHP_ME(study_coroutine_util, getCid, arginfo_study_coroutine_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
```

然后编译、安装：

```shell
make clean && make && make install
```

编写如下脚本进行测试：

```php
<?php

function task($arg)
{
	$cid = Study\Coroutine::getCid();
	echo "coroutine [{$cid}] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [{$cid}] create" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 'a');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 'b');
echo "main coroutine" . PHP_EOL;

Study\Coroutine::resume(1);
echo "main coroutine" . PHP_EOL;
Study\Coroutine::resume(2);
echo "main coroutine" . PHP_EOL;
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
~/codeDir/cppCode/study # 
```

OK，符合预期。

我们发现，我们的`resume`也是传递一个字面值，很明显，这个协程id应该是从`Study\Coroutine::create`返回的才对。所以我们改进`Study\Coroutine::create`这个接口：

```cpp
PHP_METHOD(study_coroutine_util, create)
{
    // 前面的代码没有改变

    long cid = PHPCoroutine::create(&fcc, fci.param_count, fci.params);
    RETURN_LONG(cid);
}
```

我们返回了协程的id。

OK，我们重新编译、安装：

```shell
make clean && make && make install
```

然后我们修改脚本：

```php
<?php

function task($arg)
{
	$cid = Study\Coroutine::getCid();
	echo "coroutine [{$cid}] create" . PHP_EOL;
	Study\Coroutine::yield();
	echo "coroutine [{$cid}] create" . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
$cid1 = Study\Coroutine::create('task', 'a');
echo "main coroutine" . PHP_EOL;
$cid2 = Study\Coroutine::create('task', 'b');
echo "main coroutine" . PHP_EOL;

Study\Coroutine::resume($cid1);
echo "main coroutine" . PHP_EOL;
Study\Coroutine::resume($cid2);
echo "main coroutine" . PHP_EOL;
```

然后执行：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
~/codeDir/cppCode/study # 
```

符合预期。

[下一篇：修复一些bug（一）](./《PHP扩展开发》-协程-修复一些bug（一）.md)









