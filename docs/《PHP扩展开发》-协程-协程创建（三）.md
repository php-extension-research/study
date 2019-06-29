# 协程创建（三）

[上一篇文章](./《PHP扩展开发》-协程-协程创建（二）.md)，我们成功的实现了**把一个用户空间的函数传递给创建协程的接口**的功能。这一篇文章，我们来介绍一下如何去调用这个用户空间的函数。

我们实现的接口现在变成了：

```c++
PHP_METHOD(study_coroutine_util, create)
{
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zval result;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    fci.retval = &result;
    if (zend_call_function(&fci, &fcc) != SUCCESS) {
        return;
    }

    *return_value = result;
}
```

`PHP`脚本如下：

```PHP
<?php

function task()
{
	echo "success\n";
}

Study\Coroutine::create('task');
```

执行后，结果如下：

```shell
~/codeDir/cppCode/study # php test.php 
success
~/codeDir/cppCode/study # 
```

ok，执行成功了。所以，我们这里的核心就是`zend_fcall_info`、`zend_fcall_info_cache`和`zend_call_function`。下篇文章将会介绍这三个东西。

