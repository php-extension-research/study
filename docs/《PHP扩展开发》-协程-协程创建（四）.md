# 协程创建（四）

这篇文章我们介绍下`zend_fcall_info`。我们先来看看`zend_fcall_info`的定义：

```c++
typedef struct _zend_fcall_info {
	size_t size;
	zval function_name;
	zval *retval;
	zval *params;
	zend_object *object;
	zend_bool no_separation;
	uint32_t param_count;
} zend_fcall_info;
```

`size`是结构体`zend_fcall_info`的大小，通过`sizeof(fci)`计算得到。

`function_name`是函数的名字，用来查找函数是否存在于`EG(function_table)`中。`EG(function_table)`里面包含了所有的函数。

`retval`是用来存放函数返回值的。

`params`用来存放我们需要传递给函数的参数，它是一个`zval`数组。

`object`当这个函数是属于某个类的时候会用到，指向这个类。

`no_separation`表示`zend_call_function`内部要不要释放我们的参数引用计数（一般都是传1，表示我们自己控制参数的引用计数，而`zend_call_function`只管使用即可）。

`param_count`是传递给函数的参数个数。

OK，我们通过调试来具体看看。

脚本如下：

```php
<?php

function task()
{
	echo "success\n";
}

Study\Coroutine::create('task');
```

开始调试：

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

我们在`zim_study_coroutine_util_create`出打断点：

```shell
(gdb) b zim_study_coroutine_util_create
Breakpoint 1 at 0x7ffff78d62e0: file /root/codeDir/cppCode/study/study_coroutine_util.cc, line 10.
(gdb) 
```

然后执行：

```shell
(gdb) r test.php
Starting program: /usr/local/bin/php test.php

Breakpoint 1, zim_study_coroutine_util_create (execute_data=0x7ffff761d090, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:10
(gdb) 
```

```cpp
 9│ PHP_METHOD(study_coroutine_util, create)
10├>{
11│     zend_fcall_info fci = empty_fcall_info;
12│     zend_fcall_info_cache fcc = empty_fcall_info_cache;
13│     zval result;
14│
15│     ZEND_PARSE_PARAMETERS_START(1, 1)
16│         Z_PARAM_FUNC(fci, fcc)
17│     ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
18│
19│     fci.retval = &result;
20│     if (zend_call_function(&fci, &fcc) != SUCCESS) {
21│         return;
22│     }
23│
24│     *return_value = result;
25│ }
```

我们执行到`19`行之前：

```shell
(gdb) u 19
zim_study_coroutine_util_create (execute_data=0x7ffff761d090, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:19
(gdb) 
```

```cpp
 9│ PHP_METHOD(study_coroutine_util, create)
10│ {
11│     zend_fcall_info fci = empty_fcall_info;
12│     zend_fcall_info_cache fcc = empty_fcall_info_cache;
13│     zval result;
14│
15│     ZEND_PARSE_PARAMETERS_START(1, 1)
16│         Z_PARAM_FUNC(fci, fcc)
17│     ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
18│
19├───> fci.retval = &result;
20│     if (zend_call_function(&fci, &fcc) != SUCCESS) {
21│         return;
22│     }
23│
24│     *return_value = result;
25│ }
```

此时，我们来看看`fci`里面的信息：

```shell
(gdb) p fci
$2 = {size = 56, function_name = {value = {lval = 93825009556768, dval = 4.6355713942725711e-310, counted = 0x5555565d9d20, str = 0x5555565d9d20, arr = 0x5555565d9d20, obj = 0x5555565d9d20, res = 0x555556
5d9d20, ref = 0x5555565d9d20, ast = 0x5555565d9d20, zv = 0x5555565d9d20, ptr = 0x5555565d9d20, ce = 0x5555565d9d20, func = 0x5555565d9d20, ww = {w1 = 1448975648, w2 = 21845}}, u1 = {v = {type = 6 '\006',
type_flags = 0 '\000', u = {call_info = 0, extra = 0}}, type_info = 6}, u2 = {next = 0, cache_slot = 0, opline_num = 0, lineno = 0, num_args = 0, fe_pos = 0, fe_iter_idx = 0, access_flags = 0, property_gu
ard = 0, constant_flags = 0, extra = 0}}, retval = 0x0, params = 0x0, object = 0x0, no_separation = 1 '\001', param_count = 0}
(gdb) 
```

`fci.size`的值是56，正好是`sizeof(zend_fcall_info)`的大小：

```
8 + 16 + 8 + 8 + 8 + 8(本来是zend_bool + uint32_t是5字节的，但是因为内存对齐所以占用8字节)
```

我们再来看看`function_name`，它是一个`zval`类型的结构体。因为`function_name.u1.v.type`的值是6，所以这个`zval`里面指向一个`zend_string`。所以我们需要取`fci.function_name.value.str`：

```shell
(gdb) p *fci.function_name.value.str
$5 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372043240494936, len = 4, val = "t"}
(gdb) 
```

我们发现，这个`zend_string`里面包含的字符串长度为`4`，所以我们如下查看字符串内容：

```shell
(gdb) p *fci.function_name.value.str.val@4
$7 = "task"
(gdb) 
```

OK，这就是我们的传递给我们接口的函数名字。

`retval`的话，没必要讲。

我们再来看看我们传递给`task`函数的参数`fci.params`：

```shell
(gdb) p fci.params 
$8 = (zval *) 0x0
(gdb) 
```

因为我们没有给`task`函数传递任何参数。

很显然，`fci.param_count`的值为0：

```shell
(gdb) p fci.param_count 
$9 = 0
(gdb) 
```

我们再来看看`fci.object`的值：

```shell
(gdb) p fci.object 
$10 = (zend_object *) 0x0
(gdb) 
```

因为我们这个函数不在任何类里面定义，所以很显然`object`是0。

OK，那如果我要给这个`task`函数传递参数我该怎么去做呢？比如如下脚本：

```php
<?php

function task($a, $b)
{
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}

Study\Coroutine::create('task', 'a', 'b');
```

我们需要修改一下`PHP_METHOD(study_coroutine_util, create)`这个接口：

```cpp
PHP_METHOD(study_coroutine_util, create)
{
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zval result;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_FUNC(fci, fcc)
        Z_PARAM_VARIADIC('*', fci.params, fci.param_count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    fci.retval = &result;
    if (zend_call_function(&fci, &fcc) != SUCCESS) {
        return;
    }

    *return_value = result;
}
```

我们重新编译扩展：

```shell
make clean && make && make install
```

然后执行我们的脚本：

```php
~/codeDir/cppCode/study # php test.php 
a
b
~/codeDir/cppCode/study # 
```

其中，`ZEND_PARSE_PARAMETERS_START(1, -1)`的`-1`代表没有限制传递给`Study\Coroutine::create`接口函数的最大参数个数限制，也就是可变参数。

`Z_PARAM_VARIADIC`这个宏是用来解析可变参数的，`'*'`对于`Z_PARAM_VARIADIC`实际上并没有用到。`*`表示可变参数可传或者不传递。与之对应的是`'+'`，表示可变参数至少传递一个。

OK，我们来进行调试：

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

我们在`zim_study_coroutine_util_create`出打断点：

```shell
(gdb) b zim_study_coroutine_util_create
Breakpoint 1 at 0x7ffff78d62e0: file /root/codeDir/cppCode/study/study_coroutine_util.cc, line 10.
(gdb) 
```

然后执行：

```shell
(gdb) r test.php
Starting program: /usr/local/bin/php test.php

Breakpoint 1, zim_study_coroutine_util_create (execute_data=0x7ffff761d090, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:10
(gdb)   
```

```cpp
 9│ PHP_METHOD(study_coroutine_util, create)
10├>{
11│     zend_fcall_info fci = empty_fcall_info;
12│     zend_fcall_info_cache fcc = empty_fcall_info_cache;
13│     zval result;
14│
15│     ZEND_PARSE_PARAMETERS_START(1, -1)
16│         Z_PARAM_FUNC(fci, fcc)
17│         Z_PARAM_VARIADIC('*', fci.params, fci.param_count)
18│     ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
19│
20│     fci.retval = &result;
21│     if (zend_call_function(&fci, &fcc) != SUCCESS) {
22│         return;
23│     }
24│
25│     *return_value = result;
26│ }
```

我们执行到`20`行之前：

```shell
(gdb) u 20
zim_study_coroutine_util_create (execute_data=0x7ffff761d090, return_value=0x7fffffffb0b0) at /root/codeDir/cppCode/study/study_coroutine_util.cc:20
(gdb) 
```

此时，我们来查看一下`fci.param_count`：

```shell
(gdb) p fci.param_count
$1 = 2
(gdb) 
```

因为我们传递了`2`个可变参数，所以这里是`2`。

我们再来看看`fci.params`：

```shell
(gdb) p *fci.params
$3 = {value = {lval = 140737353162016, dval = 6.9533491283979037e-310, counted = 0x7ffff7f11d20, str = 0x7ffff7f11d20, arr = 0x7ffff7f11d20, obj = 0x7ffff7f11d20, res = 0x7ffff7f11d20, ref = 0x7ffff7f11d2
0, ast = 0x7ffff7f11d20, zv = 0x7ffff7f11d20, ptr = 0x7ffff7f11d20, ce = 0x7ffff7f11d20, func = 0x7ffff7f11d20, ww = {w1 = 4159773984, w2 = 32767}}, u1 = {v = {type = 6 '\006', type_flags = 0 '\000', u =
{call_info = 0, extra = 0}}, type_info = 6}, u2 = {next = 0, cache_slot = 0, opline_num = 0, lineno = 0, num_args = 0, fe_pos = 0, fe_iter_idx = 0, access_flags = 0, property_guard = 0, constant_flags = 0
, extra = 0}}
(gdb) 
```

我们打印第一个参数：

```shell
(gdb) p *fci.params[0].value.str.val@1
$21 = "a"
(gdb) 
```

再打印第二个参数：

```shell
(gdb) p *fci.params[1].value.str.val@1
$22 = "b"
(gdb) 
```

OK，分析完毕。





