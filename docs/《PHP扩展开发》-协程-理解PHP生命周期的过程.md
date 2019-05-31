# 理解PHP生命周期的过程

我们在整理`study.cc`文件的时候，看到了一下函数：

```c++
PHP_MINIT_FUNCTION(study)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(study)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(study)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(study)
{
	return SUCCESS;
}
```

这几个函数是伴随着PHP生命周期来执行的。所以，我们需要知道`PHP`的生命周期。

PHP生命周期有`5`个阶段：

```
1、模块初始化阶段
2、请求初始化阶段
3、执行PHP脚本阶段
4、请求关闭阶段
5、模块关闭阶段
```

OK，我们现在来测试一下这几个函数。修改这几个函数的内容：

```c++
PHP_MINIT_FUNCTION(study)
{
	php_printf("MINIT\n");
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(study)
{
	php_printf("MSHUTDOWN\n");
	return SUCCESS;
}

PHP_RINIT_FUNCTION(study)
{
	php_printf("RINIT\n");
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(study)
{
	php_printf("RSHUTDOWN\n");
	return SUCCESS;
}
```

然后，在扩展的根目录下面创建一个文件，叫做`test.php`：

```shell
~/codeDir/cppCode/study # touch test.php
```

内容如下：

```php
<?php

echo "execute the script\n";
```

OK，我们重新编译一下扩展：

```shell
~/codeDir/cppCode/study # ./make.sh 
```

然后在配置文件中开启这个扩展：

```ini
extension=study.so
```

现在，我们开启`PHP`内置的服务器：

```shell
~/codeDir/cppCode/study # php -S localhost:8080
MINIT
PHP 7.3.5 Development Server started at Fri May 31 03:23:25 2019
Listening on http://localhost:8080
Document root is /root/codeDir/cppCode/study
Press Ctrl-C to quit.

```

我们现在所处的目录就是这个服务器的根目录。我们请求这个服务器的时候，服务器会在这个目录里面查找资源。

OK，可以看到，我们在开启服务器的时候，打印出了`MINIT`。现在，我们另开一个终端去请求一下这个服务器：

```shell
~/codeDir/phpCode/test # curl localhost:8080/test.php
RINIT
execute the script
RSHUTDOWN
~/codeDir/phpCode/test # 
```

我们再次请求一下：

```shell
~/codeDir/phpCode/test # curl localhost:8080/test.php
RINIT
execute the script
RSHUTDOWN
~/codeDir/phpCode/test # 
```

发现，分别打印出了`RINIT`、`execute the script`、`RSHUTDOWN`。

然后，服务器这一边的输出内容如下：

```shell
[Fri May 31 03:32:00 2019] 127.0.0.1:46782 [200]: /test.php
[Fri May 31 03:32:23 2019] 127.0.0.1:46784 [200]: /test.php
```

然后，我们按`ctrl + c`关闭服务器：

```shell
^CMSHUTDOWN
~/codeDir/cppCode/study # 
```

此时，打印了`MSHUTDOWN`。因此，我们可以很直观的感受到，`PHP`的生命周期过程是：

```
MINIT
RINIT
execute the script
RSHUTDOWN
RINIT
.
.
.
RSHUTDOWN
MSHUTDOWN
```

这几个生命周期做什么事情，很多内核分析文章有讲。我们这里只需要大致了解一个流程即可。