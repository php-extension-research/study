# 修复一些bug（一）

开发到这里，我们需要停下来修复一些bug。我们通过具体的脚本例子来触发这个bug，然后我们通过调试分析，来修复这个bug。或许有些读者在阅读的时候发现了明显的bug，实际上，这些bug是故意留出来的，因为我们这一系列的教程是渐进式的，不会一气呵成。否则就失去了意义，那样只会和源码分析没啥区别。

OK，我们来看这个脚本：

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
Study\Coroutine::resume($cid2);
```

和之前脚本的区别在于最后几行，我调用了2次`resume($cid2)`。

OK，我们来执行一下脚本：

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
Segmentation fault
~/codeDir/cppCode/study # 
```

我们发现，报了一错`Segmentation fault`。

我们来调试一下：

```shell
~/codeDir/cppCode/study # cgdb php
GNU gdb (GDB) 8.2
Copyright (C) 2018 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-alpine-linux-musl".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from php...(no debugging symbols found)...done.
(gdb) 
```

然后执行脚本：

```shell
(gdb) r test.php
This GDB was configured as "x86_64-alpine-linux-musl".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from php...(no debugging symbols found)...done.
(gdb) r test.php 
Starting program: /usr/local/bin/php test.php
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine

Program received signal SIGSEGV, Segmentation fault.
zim_study_coroutine_util_resume (execute_data=0x7ffff761d110, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:54
(gdb) 
```

我们发现报错地方是`study_coroutine_util.cc:54`。对应的代码是：

```cpp
Coroutine* co = coroutine_iterator->second;
```

很容易想到问题肯定是出在了`coroutine_iterator->second`上面。

OK，我们在函数`zim_study_coroutine_util_resume`打一个断点：

```cpp
(gdb) b zim_study_coroutine_util_resume
Breakpoint 1 at 0x7ffff78d1b8a: file /root/codeDir/cppCode/study/study_coroutine_util.cc, line 45.
(gdb) 
```

然后我们重新执行脚本：

```shell
(gdb) r test.php 
The program being debugged has been started already.
Start it from the beginning? (y or n) y
The program being debugged has been started already.
Starting program: /usr/local/bin/php test.php
main coroutine
coroutine [1] create
main coroutine
coroutine [2] create
main coroutine

Breakpoint 1, zim_study_coroutine_util_resume (execute_data=0x7ffff761d110, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:45
(gdb) 
```

我们继续执行：

```shell
(gdb) c
coroutine [1] create
main coroutine
Continuing.

Breakpoint 1, zim_study_coroutine_util_resume (execute_data=0x7ffff761d110, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:45
(gdb) 
```

我们再继续执行：

```shell
(gdb) c
Continuing.
coroutine [2] create
main coroutine

Breakpoint 1, zim_study_coroutine_util_resume (execute_data=0x7ffff761d110, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:45
(gdb) 
```

```shell
44│ PHP_METHOD(study_coroutine_util, resume)
45├>{
46│     zend_long cid = 0;
47│
48│     ZEND_PARSE_PARAMETERS_START(1, 1)
49│         Z_PARAM_LONG(cid)
50│     ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
51│
52│     auto coroutine_iterator = user_yield_coros.find(cid);
53│
54│     Coroutine* co = coroutine_iterator->second;
55│     user_yield_coros.erase(cid);
56│     co->resume();
57│     RETURN_TRUE;
```

OK，这个时候，停在了最后一行代码：

```php
Study\Coroutine::resume($cid2);
```

里面了。

我们执行到`54`行之前（不会执行54行的代码），这是报错的地方：

```shell
(gdb) u 54
zim_study_coroutine_util_resume (execute_data=0x7ffff761d110, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:54
(gdb) 
```

```shell
52│     auto coroutine_iterator = user_yield_coros.find(cid);
53│
54├───> Coroutine* co = coroutine_iterator->second;
55│     user_yield_coros.erase(cid);
56│     co->resume();
57│     RETURN_TRUE;
58│ }
```

此时，我们打印`coroutine_iterator`的值：

```shell
(gdb) p coroutine_iterator
$2 = {<std::__detail::_Node_iterator_base<std::pair<long const, Study::Coroutine*>, false>> = {_M_cur = 0x0}, <No data fields>}
(gdb) 
```

发现`_M_cur = 0x0`，所以是没有取出东西来。

没有取出东西来的原因是我们在每次`resume`一个协程的时候，会把对应的协程从`user_yield_coros`里面删除，所以我们接着第二次`resume`这个协程的时候，就无法找到这个协程了。除非在第二次`resume`这个协程之前，又`yield`这个协程了，重新把这个协程添加进去了`user_yield_coros`里面。

所以，这里我们需要对`coroutine_iterator`进行判断。我们修改一下`PHP_METHOD(study_coroutine_util, resume)`这个接口：

```cpp
if (coroutine_iterator == user_yield_coros.end())
{
    php_error_docref(NULL, E_WARNING, "resume error");
    RETURN_FALSE;
}
```

OK，我们编译、安装：

```shell
make clean && make && make install
```

然后在执行脚本：

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

Warning: Study\Coroutine::resume(): resume error in /root/codeDir/cppCode/study/test.php on line 21
~/codeDir/cppCode/study # 
```

OK，我们成功的报出了`Warning`，不至于让脚本因为无法`resume`一个协程而停止执行

[下一篇：修复一些bug（二）](./《PHP扩展开发》-协程-修复一些bug（二）.md)







