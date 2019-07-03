# 修复一些bug（三）

这篇文章，我们来修复一个`Study\Coroutine::getCid()`的bug。

测试脚本如下：

```php
<?php

$cid = Study\Coroutine::getCid();
echo "coroutine [{$cid}] create" . PHP_EOL;
```

也就是说，我们在非协程环境下调用了`Study\Coroutine::getCid()`。

我们来执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
Segmentation fault
~/codeDir/cppCode/study # 
```

报错了。

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
13│ public:
14│     static std::unordered_map<long, Coroutine*> coroutines;
15│
16│     static void* get_current_task();
17│     static long create(coroutine_func_t fn, void* args = nullptr);
18│     void* get_task();
19│     static Coroutine* get_current();
20│     void set_task(void *_task);
21│     void yield();
22│     void resume();
23│
24│     inline long get_cid()
25│     {
26├─666666> return cid;
27│     }
```

是在调用`Study::Coroutine::get_cid()`的时候报错的。

我们在`zim_study_coroutine_util_getCid`处打一个断点：

```shell
(gdb) b zim_study_coroutine_util_getCid
Breakpoint 1 at 0x7ffff78d1f73: file /root/codeDir/cppCode/study/study_coroutine_util.cc, line 71.
(gdb) 
```

然后重新运行脚本：

```shell
(gdb) r test.php 
The program being debugged has been started already.
Start it from the beginning? (y or n) y
The program being debugged has been started already.
Starting program: /usr/local/bin/php test.php

Breakpoint 1, zim_study_coroutine_util_getCid (execute_data=0x7ffff761d0f0, return_value=0x7ffff761d090) at /root/codeDir/cppCode/study/study_coroutine_util.cc:71
(gdb) 
```

```cpp
 69│ PHP_METHOD(study_coroutine_util, getCid)
 70│ {
 71├───> Coroutine* co = Coroutine::get_current();
 72│     RETURN_LONG(co->get_cid());
 73│ }
```

继续：

```shell
(gdb) n
(gdb) 
```

```cpp
 69│ PHP_METHOD(study_coroutine_util, getCid)
 70│ {
 71│     Coroutine* co = Coroutine::get_current();
 72├───> RETURN_LONG(co->get_cid());
 73│ }
```

打印一下这个`co`：

```shell
(gdb) p co
$1 = (Study::Coroutine *) 0x0
(gdb) 
```

我们发现，因为我们的`Study::Coroutine::current`在初始化的时候，给了初始值：

```cpp
Coroutine* Coroutine::current = nullptr;
```

所以，当然是`0x0`了。因此我们无法调用`get_cid()`。

所以，我们只需要判断一下`Coroutine* co = Coroutine::get_current();`取出来的`co`是否为`nullptr`即可修复这个bug：

```cpp
Coroutine* co = Coroutine::get_current();
if (co == nullptr)
{
  	RETURN_LONG(-1);
}
RETURN_LONG(co->get_cid());
```

OK，我们编译、安装：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
coroutine [-1] create
~/codeDir/cppCode/study # 
```

符合预期。



