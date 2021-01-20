# 修复一些bug（十）

这一篇文章，我们修复一个比较难肉眼看出来的`bug`，必须利用内存检测工具才好看出来。

测试脚本如下：

```php
<?php

```

对的，测试脚本什么都不做。我们执行测试脚本：

```shell
~/codeDir/cppCode/study # php test.php
~/codeDir/cppCode/study #
```

看似没有问题对吧，但是，我们通过`gdb`就会看到段错误：

```shell
(gdb) r test.php
Starting program: /usr/local/bin/php test.php

Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7f9040d in ?? () from /lib/ld-musl-x86_64.so.1
(gdb) bt
#0  0x00007ffff7f9040d in ?? () from /lib/ld-musl-x86_64.so.1
#1  0x00007ffff7f90537 in ?? () from /lib/ld-musl-x86_64.so.1
#2  0x0000000000000002 in ?? ()
#3  0x00007ffff7f90633 in ?? () from /lib/ld-musl-x86_64.so.1
#4  0x0000000000000e20 in ?? ()
#5  0x00005555564d3ed0 in ?? ()
#6  0x0000000000000060 in ?? ()
#7  0x00005555564d3e70 in ?? ()
#8  0x00007ffff7ffba20 in ?? () from /lib/ld-musl-x86_64.so.1
#9  0x0000000000000000 in ?? ()
(gdb)
```

而且通过调用栈是看不出`bug`的，这时候，我们得使用内存分析工具`valgrid`才可以看出问题。安装`valgrind`后，执行如下命令：

```shell
ZEND_DONT_UNLOAD_MODULES=1 USE_ZEND_ALLOC=0 valgrind --leak-check=full --show-reachable=yes --track-origins=yes php -dextension=study.so study.php

==71427== LEAK SUMMARY:
==71427==    definitely lost: 16 bytes in 1 blocks
==71427==    indirectly lost: 0 bytes in 0 blocks
==71427==      possibly lost: 0 bytes in 0 blocks
==71427==    still reachable: 81,482 bytes in 41 blocks
==71427==         suppressed: 0 bytes in 0 blocks
==71427==
==71427== For counts of detected and suppressed errors, rerun with: -v
==71427== ERROR SUMMARY: 282 errors from 27 contexts (suppressed: 0 from 0)
~/codeDir/cppCode/study #
```

我们看这一行：

```shell
==71427==    definitely lost: 16 bytes in 1 blocks
```

这个表示我们有`16`字节的内存泄漏。我们看其他输出，可以找到是哪个地方内存泄漏了：

```shell
==71427== 16 bytes in 1 blocks are definitely lost in loss record 10 of 27
==71427==    at 0x489C72A: malloc (vg_replace_malloc.c:299)
==71427==    by 0x534004F: study_coroutine_server_coro_init() (study_server_coro.cc:161)
==71427==    by 0x533E14D: zm_startup_study(int, int) (study.cc:55)
==71427==    by 0x60000C: zend_startup_module_ex (in /usr/local/bin/php)
==71427==    by 0x60009B: ??? (in /usr/local/bin/php)
==71427==    by 0x60C621: zend_hash_apply (in /usr/local/bin/php)
==71427==    by 0x600379: zend_startup_modules (in /usr/local/bin/php)
==71427==    by 0x59D232: php_module_startup (in /usr/local/bin/php)
==71427==    by 0x6839BC: ??? (in /usr/local/bin/php)
==71427==    by 0x265C3C: ??? (in /usr/local/bin/php)
==71427==    by 0x401E13C: (below main) (in /lib/ld-musl-x86_64.so.1)
```

我们看到，在我们的文件`study_server_coro.cc`的`161`行有内存泄漏。这一行在我这里对应：

```cpp
zval *zsock = (zval *)malloc(sizeof(zval));
```

可以看到，我们在模块初始化的时候，在堆上面分配了一个`zval`的内存，但是没有释放。而一个`zval`的大小刚好就是`16`字节。

我们需要把代码修改为：

```cpp
void study_coroutine_server_coro_init()
{
    zval zsock; // 修改的地方

    INIT_NS_CLASS_ENTRY(study_coroutine_server_coro_ce, "Study", "Coroutine\\Server", study_coroutine_server_coro_methods);
    study_coroutine_server_coro_ce_ptr = zend_register_internal_class(&study_coroutine_server_coro_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property(study_coroutine_server_coro_ce_ptr, ZEND_STRL("zsock"), &zsock, ZEND_ACC_PUBLIC); // 修改的地方
    zend_declare_property_string(study_coroutine_server_coro_ce_ptr, ZEND_STRL("host"), "", ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coroutine_server_coro_ce_ptr, ZEND_STRL("port"), -1, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coroutine_server_coro_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coroutine_server_coro_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);
}
```

然后重新编译、安装扩展：

```shell
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study #
```

然后重新用`gdb`执行脚本：

```shell
(gdb) r test.php
Starting program: /usr/local/bin/php test.php
[Inferior 1 (process 75730) exited normally]
(gdb)
```

符合预期，`bug`解决了。

[下一篇：重构Channel（一）](./《PHP扩展开发》-协程-重构Channel（一）.md)
