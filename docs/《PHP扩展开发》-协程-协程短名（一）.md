# 协程短名（一）

之前我们调用协程类提供的接口，我们需要些很长的命名空间`Study::Coroutine`。现在我们来优化一下，希望变成这样：

```php
Co::create()
```

的风格。

我们修改`study_coroutine_util.cc`文件里的`study_coroutine_util_init()`函数，变为：

```cpp
void study_coroutine_util_init()
{
    INIT_NS_CLASS_ENTRY(study_coroutine_ce, "Study", "Coroutine", study_coroutine_util_methods);
    study_coroutine_ce_ptr = zend_register_internal_class(&study_coroutine_ce TSRMLS_CC); // Registered in the Zend Engine
    zend_register_class_alias("Co", study_coroutine_ce_ptr); // 新增的代码
}
```

OK，我们编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

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
    Co::defer('deferFunc1');
    Co::defer('deferFunc2');
    Co::defer('deferFunc3');
    echo "task coroutine end" . PHP_EOL;
}

$cid1 = Co::create('task');
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





