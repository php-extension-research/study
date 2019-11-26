# hook原来的sleep

现在，我们进入一个全新的主题，讲解如何替换掉`PHP`原来那些阻塞的函数。已达到不修改历史代码，就可以直接协程化我们的代码。

我们这篇文章需要实现的方法如下：

```php
Study\Runtime\enableCoroutine()
```

首先，我们创建两个文件`study_runtime.h`、`study_runtime.cc`。

其中，`study_runtime.h`文件的内容如下：

```cpp
#ifndef STUDY_RUNTIME_H
#define STUDY_RUNTIME_H

#include "php_study.h"

#endif /* STUDY_RUNTIME_H */
```

其中，`study_runtime.cc`文件的内容如下：

```cpp
#include "study_runtime.h"
```

然后修改我们的`config.m4`文件，增加`study_runtime.cc`为需要编译的源文件：

```shell
study_source_file="\
    study.cc \
    study_coroutine.cc \
    study_coroutine_util.cc \
    src/coroutine/coroutine.cc \
    src/coroutine/context.cc \
    ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
    ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE} \
    src/socket.cc \
    src/log.cc \
    src/error.cc \
    src/core/base.cc \
    src/coroutine/socket.cc \
    src/timer.cc \
    study_coroutine_channel.cc \
    src/coroutine/channel.cc \
    study_coroutine_socket.cc \
    study_coroutine_server.cc \
    study_runtime.cc
"
```

然后和往常一样，我们需要先实现`PHP`类`Study\Runtime`。

在文件`study_runtime.cc`里面，我们实现我们的`enableCoroutine`方法。首先，定义一下这个方法的参数。目前我们不需要传递参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_runtime_void, 0, 0, 0)
ZEND_END_ARG_INFO()
```

然后是`enableCoroutine`方法的大体框架：

```cpp
extern PHP_METHOD(study_coroutine_util, sleep);
static void hook_func(const char *name, size_t name_len, zif_handler handler);

static PHP_METHOD(study_runtime, enableCoroutine)
{
    hook_func(ZEND_STRL("sleep"), zim_study_coroutine_util_sleep);
}
```

(后面我们会去实现`hook_func`这个函数)

这里，我们的意图是把`PHP`原来的`sleep`函数替换成`Study\Coroutine::sleep`这个方法。`zim_study_coroutine_util_sleep`实际上就是我们在文件`study_coroutine_util.cc`中定义的`PHP_METHOD(study_coroutine_util, sleep)`展开后的结果。

然后，收集`enableCoroutine`到结构体`zend_function_entry`里面：

```cpp
static const zend_function_entry study_runtime_methods[] =
{
    PHP_ME(study_runtime, enableCoroutine, arginfo_study_runtime_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

然后创建一个模块初始化函数来注册`PHP`类`Study\Runtime`：

```cpp
/**
 * Define zend class entry
 */
zend_class_entry study_runtime_ce;
zend_class_entry *study_runtime_ce_ptr;

void study_runtime_init()
{
    INIT_NS_CLASS_ENTRY(study_runtime_ce, "Study", "Runtime", study_runtime_methods);
    study_runtime_ce_ptr = zend_register_internal_class(&study_runtime_ce TSRMLS_CC); // Registered in the Zend Engine
}
```

然后，我们在`study.cc`文件的`PHP_MINIT_FUNCTION(study)`函数里面调用`study_runtime_init`：

```cpp
PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    study_coro_server_init(module_number);
    study_coro_channel_init();
    study_coro_socket_init(module_number);
    study_runtime_init(); // 新增的代码
    return SUCCESS;
}
```

我们需要在`php_study.h`里面对`study_runtime_init`函数进行声明：

```cpp
void study_coroutine_util_init();
void study_coro_server_init(int module_number);
void study_coro_channel_init();
void study_coro_socket_init(int module_number);
void study_runtime_init(); // 新增的代码
```

`OK`，我们完成了`Study\Runtime`类的注册。

最后，我们需要实现`hook_func`：

```cpp
static void hook_func(const char *name, size_t name_len, zif_handler new_handler)
{
    zend_function *ori_f = (zend_function *) zend_hash_str_find_ptr(EG(function_table), name, name_len);
    ori_f->internal_function.handler = new_handler;
}
```

代码其实比较简单，就是先去全局的`EG(function_table)`里面查找`sleep`名字对应的`zend_function`，然后把它的`handler`换成我们新的`new_handler`即可。也就是说，`PHP`内核实现的`C`函数实际上是会以函数指针的形式保存在`zend_function.internal_function.handler`里面。

然后，我们重新编译、安装扩展：

```shell
phpize --clean && phpize && ./configure && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

`OK`，编译、安装扩展成功。

我们来编写测试脚本：

```php
<?php

study_event_init();

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
    sleep(1);
    var_dump(Study\Coroutine::getCid());
});

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
});

study_event_wait();
```

这个脚本没有开启`hook`，执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(1)
int(2)
```

符合预期。进程因为第一个协程调用了阻塞的`sleep`函数，所以导致整个进程阻塞了起来，所以打印是顺序的。

我们现在来开启一下`hook`功能：

```php
<?php

study_event_init();

Study\Runtime::enableCoroutine();

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
    sleep(1);
    var_dump(Study\Coroutine::getCid());
});

Sgo(function ()
{
    var_dump(Study\Coroutine::getCid());
});

study_event_wait();
```

结果如下：

```shell
~/codeDir/cppCode/study # php test.php
int(1)
int(2)
int(1)
```

因为我们已经把`PHP`原先的`sleep`函数替换成了`Study\Coroutine::sleep`方法，所以，进程不会阻塞起来，会在调用`sleep`之后立马切换到第二个协程。
