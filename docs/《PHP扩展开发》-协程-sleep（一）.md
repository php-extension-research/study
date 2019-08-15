# sleep（一）

这篇文章，我们来实现一下我们协程的`sleep`接口。在文件`study_coroutine_util.cc`中，我们先定义一下我们的接口参数：

```cpp
ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_sleep, 0, 0, 1)
    ZEND_ARG_INFO(0, seconds)
ZEND_END_ARG_INFO()
```

根据命名我们知道，我们希望这个`sleep`接口接收的单位是秒。（但是我们可以定时的精度是毫秒级别的，即可以传入`0.1`这样的参数）

然后我们实现一下我们的接口：

```cpp
PHP_METHOD(study_coroutine_util, sleep)
{
    double seconds;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_DOUBLE(seconds)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (UNEXPECTED(seconds < 0.001))
    {
        php_error_docref(NULL, E_WARNING, "Timer must be greater than or equal to 0.001");
        RETURN_FALSE;
    }

    PHPCoroutine::sleep(seconds);
    RETURN_TRUE;
}
```

其中

```cpp
if (UNEXPECTED(seconds < 0.001))
{
    php_error_docref(NULL, E_WARNING, "Timer must be greater than or equal to 0.001");
  	RETURN_FALSE;
}
```

就是说，我们定时的最小时间不可以小于`0.001`。（注意，在编程的过程中，尽可能的不要使用诸如`0.001`这样的**魔数**，一般，我们会定义一个宏，来替换掉`0.001`）

然后，我们调用

```cpp
PHPCoroutine::sleep(seconds);
```

接着，我们在`study_coroutine.h`文件里面的类`PHPCoroutine`里面声明一下这个方法：

```cpp
public:
    static int sleep(double seconds);
```

然后，我们在`study_coroutine.cc`里面进行实现：

```cpp
int PHPCoroutine::sleep(double seconds)
{
    std::cout << seconds << endl;
    return 0;
}
```

目前，我们只是简单的输出`PHP`脚本传递过来的参数。

实现协程的`sleep`。

定义完了`sleep`接口之后，我们需要注册一下：

```cpp
static const zend_function_entry study_coroutine_util_methods[] =
{
// 省略了其他之前注册的方法
    PHP_ME(study_coroutine_util, sleep, arginfo_study_coroutine_sleep, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};
```

然后，我们重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
```

然后编写测试脚本：

```php
<?php

SCo::sleep(1);
SCo::sleep(0.1);
SCo::sleep(0.001);
SCo::sleep(0.0001);
```

执行结果如下：

```shell
~/codeDir/cppCode/study # php test.php 
1
0.1
0.001

Warning: Study\Coroutine::sleep(): Timer must be greater than or equal to 0.001 in /root/codeDir/cppCode/study/test.php on line 6
~/codeDir/cppCode/study # 
```

`OK`，测试成功。

接下来的文章，我将会继续带着大家去实现`PHPCoroutine::sleep`。

