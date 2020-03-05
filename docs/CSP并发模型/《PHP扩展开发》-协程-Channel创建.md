# Channel创建

这篇文章，我们来实现一个新的类`Coroutine\Channel`。

首先，我们需要实现`Channel`创建的接口：

```php
Coroutine\Channel->__construct(int $capacity = 1)
```

其中`capacity`是它的容量，即最大存储的数据个数。

`OK`，我们创建文件`study_coroutine_channel.cc`和`study_coroutine_channel.h`。然后修改`config.m4`文件，并且重新生成一份`Makefile`。

现在来声明一下这个接口的参数，在文件`study_coroutine_channel.cc`里面：

```cpp
#include "study_coroutine_channel.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_construct, 0, 0, 0)
    ZEND_ARG_INFO(0, capacity)
ZEND_END_ARG_INFO()
```

然后定义这个构造函数接口：

```cpp
/**
 * Define zend class entry
 */
zend_class_entry study_coro_channel_ce;
zend_class_entry *study_coro_channel_ce_ptr;

static PHP_METHOD(study_coro_channel, __construct)
{
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
}
```

这个代码很简单，就是去接收参数`capacity`的值，然后更新到`PHP`对象的属性里面。

现在，我们注册这个方法：

```cpp
static const zend_function_entry study_coro_channel_methods[] =
{
    PHP_ME(study_coro_channel, __construct, arginfo_study_coro_channel_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_FE_END
};
```

然后，注册这个`Channel`类：

```cpp
void study_coro_channel_init()
{
    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}
```

然后，我们在`PHP`模块初始化的地方调用这个`study_coro_channel_init`方法。在文件`study.cc`里面：

```cpp
PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    study_coroutine_server_coro_init();
    study_coro_channel_init(); // 新增的一行
    return SUCCESS;
}
```

然后在文件`php_study.h`里面声明这个函数：

```cpp
void study_coroutine_util_init();
void study_coroutine_server_coro_init();
void study_coro_channel_init(); // 新增的一行
```

重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # phpize --clean ; phpize ; ./configure
~/codeDir/cppCode/study # make clean ; make -j4 ; make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

`OK`，编译成功。我们编写测试脚本：

```php
<?php

$chan = new Study\Coroutine\Channel();
var_dump($chan);

$chan = new Study\Coroutine\Channel(-1);
var_dump($chan);

$chan = new Study\Coroutine\Channel(2);
var_dump($chan);
```

执行结果：

```shell
~/codeDir/cppCode/study # php examples/create_channel.php
object(Study\Coroutine\Channel)#1 (1) {
  ["capacity"]=>
  int(1)
}
object(Study\Coroutine\Channel)#2 (1) {
  ["capacity"]=>
  int(1)
}
object(Study\Coroutine\Channel)#1 (1) {
  ["capacity"]=>
  int(2)
}
~/codeDir/cppCode/study #
```

符合预期。

[下一篇：实现Channel基础类](./《PHP扩展开发》-协程-实现Channel基础类.md)
