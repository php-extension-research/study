# server创建（一）

这一篇文章，我们正式进入了`Server`的开发阶段。我们将会创建一个全新的类`Study\Coroutine\Server`。

需要注意的一点是，我们的`Coroutine\Server`类和`Swoole`的`Coroutine\Server`类实现方式是完全不同的，因为`Swoole`是通过`PHP`代码来实现`Coroutine\Server`的，但是我们是通过`Cpp`代码来实现的。因为我们目前还没有去实现`Coroutine\Socket`这个类。后面我们会去实现，然后再重构我们的`Coroutine\Server`代码。

`OK`，我们创建一个新的文件`study_server_coro.h`，文件内容如下：

```cpp
#ifndef STUDY_SERVER_CORO_H
#define STUDY_SERVER_CORO_H

#include "php_study.h"

#endif	/* STUDY_SERVER_CORO_H */
```

然后，我们再创建一个文件`study_server_coro.cc`，文件内容如下：

```cpp
#include "study_server_coro.h"
```

因为我们增加了一个需要被编译的文件，所以我们需要在`config.m4`文件里面指明。我们修改`config.m4`文件：

```shell
study_source_file="\
    study.cc \
    study_coroutine.cc \
    study_coroutine_util.cc \
    src/coroutine/coroutine.cc \
    src/coroutine/context.cc \
    ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
    ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE} \
    study_server_coro.cc
"
```

`OK`，我们现在去实现我们的服务器类。

首先我们在文件`study_server_coro.cc`里面定义用于模块初始化的方法：

```cpp
/**
 * Define zend class entry
 */
zend_class_entry study_coroutine_server_coro_ce;
zend_class_entry *study_coroutine_server_coro_ce_ptr;

void study_coroutine_server_coro_init()
{
    INIT_NS_CLASS_ENTRY(study_coroutine_server_coro_ce, "Study", "Coroutine\\Server", NULL);
    study_coroutine_server_coro_ce_ptr = zend_register_internal_class(&study_coroutine_server_coro_ce TSRMLS_CC); // Registered in the Zend Engine
}
```

然后，我们在`php_study.h`文件里面去声明这个方法：

```cpp
void study_coroutine_server_coro_init();
```

接着，我们在`study.cc`文件里面的`PHP_MINIT_FUNCTION(study)`方法里面去调用这个方法：

```cpp
PHP_MINIT_FUNCTION(study)
{
	study_coroutine_util_init();
	study_coroutine_server_coro_init(); // 新增加的代码
	return SUCCESS;
}
```

`OK`，我们现在完成了`Study\Coroutine\Server`类的注册。

因为我们修改了`config.m4`文件，所以我们需要重新执行`phpize`命令：

```shell
~/codeDir/cppCode/study # phpize --clean && phpize && ./configure
```

接着编译、安装扩展：

```shell
~/codeDir/cppCode/study # make && make install
```

然后编写测试脚本：

```php
<?php

$serv = new Study\Coroutine\Server;
var_dump($serv);
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
object(Study\Coroutine\Server)#1 (0) {
}
~/codeDir/cppCode/study # 
```

打印成功。

