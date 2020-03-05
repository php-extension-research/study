# 引入libuv

大家好，从这篇文章开始，就进入了我们`PHP`协程扩展开发的网络阶段了。为了支持网络部分，是需要一些基础的数据结构和工具的。例如堆、定时器等等。最开始的时候，我打算直接把`Swoole`作为动态连接库来供我们的协程扩展使用，但是想了一下，如果直接用的话，感觉和直接用`Swoole`没啥区别了，所以我就去找其他的库，看了下`Node.js`的一个库`libuv`比较全面，并且别人也基于`libuv`为`Node.js`实现了协程，所以我就打算用这个库了。

`OK`，开始我们的教程。

首先，我们需要去把`libuv`源码编译成一个动态连接库。过程如下：

下载`libuv`源代码：

```shell
git clone https://github.com/libuv/libuv.git
```

编译：

```shell
~/codeDir/cppCode/libuv # sh autogen.sh
+ libtoolize --copy
+ aclocal -I m4
+ autoconf
+ automake --add-missing --copy
configure.ac:38: installing './ar-lib'
configure.ac:25: installing './compile'
configure.ac:22: installing './config.guess'
configure.ac:22: installing './config.sub'
configure.ac:21: installing './install-sh'
configure.ac:21: installing './missing'
Makefile.am: installing './depcomp'

~/codeDir/cppCode/libuv # ./configure
省略了其他输出
checking that generated files are newer than configure... done
configure: creating ./config.status
config.status: creating Makefile
config.status: creating libuv.pc
config.status: executing depfiles commands
config.status: executing libtool commands

~/codeDir/cppCode/libuv # make
省略了其他输出
  CC       src/unix/libuv_la-proctitle.lo
  CC       src/unix/libuv_la-sysinfo-loadavg.lo
  CCLD     libuv.la
ar: `u' modifier ignored since `D' is the default (see `U')

~/codeDir/cppCode/libuv # make check
省略了其他输出
ok 365 - we_get_signals_mixed
PASS: test/run-tests
=============
1 test passed
=============
make[1]: Leaving directory '/root/codeDir/cppCode/libuv'

~/codeDir/cppCode/libuv # make install
省略其他输出
See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------
 ./install-sh -c -d '/usr/local/include'
 /usr/bin/install -c -m 644 include/uv.h '/usr/local/include'
 ./install-sh -c -d '/usr/local/lib/pkgconfig'
 /usr/bin/install -c -m 644 libuv.pc '/usr/local/lib/pkgconfig'
 ./install-sh -c -d '/usr/local/include/uv'
 /usr/bin/install -c -m 644 include/uv/errno.h include/uv/threadpool.h include/uv/version.h include/uv/unix.h include/uv/linux.h '/usr/local/include/uv'
```

（如果在编译的时候报错说某些工具没找到，可以自行谷歌安装）

我们编译完成后，会看到一些头文件被安装了：

```shell
ls /usr/local/include/
uv.h     uv/
```

并且，也生成了对应的动态链接库`libuv.so`：

```shell
~/codeDir/cppCode/libuv # ls /usr/local/lib/
libswoole.so        libswoole.so.4.4.2  libuv.a             libuv.la            libuv.so            libuv.so.1          libuv.so.1.0.0      perl5               php                 pkgconfig
```

`OK`，此时我们就需要在编译我们协程扩展的时候指定`libuv.so`动态链接库。修改`config.m4`文件：

```shell
if test "$PHP_STUDY" != "no"; then
########新增加的2行
    PHP_ADD_LIBRARY_WITH_PATH(uv, /usr/local/lib/, STUDY_SHARED_LIBADD)
    PHP_SUBST(STUDY_SHARED_LIBADD)
########新增加的2行

    PHP_ADD_LIBRARY(pthread)
    STUDY_ASM_DIR="thirdparty/boost/asm/"
    CFLAGS="-Wall -pthread $CFLAGS"
```

其中

`PHP_ADD_LIBRARY_WITH_PATH`用来添加链接库，并且指明我们的动态链接库的路径是`/usr/local/lib/`。

`PHP_SUBST`用来启用扩展的共享构建。

`OK`，修改完了`config.m4`文件之后，我们需要重新执行`phpize`命令才行（因为在`Makefile`里面需要去体现出我们用到了动态链接库`libuv.so`）。过程如下：

```shell
~/codeDir/cppCode/study # phpize --clean
~/codeDir/cppCode/study # phpize 
~/codeDir/cppCode/study # ./configure
```

增加了`config.m4`文件的这两行之后，会在`./configure`命令生成的`Makefile`里面得到体现：

```makefile
STUDY_SHARED_LIBADD = -Wl,-rpath,/usr/local/lib/ -L/usr/local/lib/ -luv

./study.la: $(shared_objects_study) $(STUDY_SHARED_DEPENDENCIES)
	$(LIBTOOL) --mode=link $(CXX) $(COMMON_FLAGS) $(CFLAGS_CLEAN) $(EXTRA_CFLAGS) $(LDFLAGS)  -o $@ -export-dynamic -avoid-version -prefer-pic -module -rpath $(phplibdir) $(EXTRA_LDFLAGS) $(shared_objects_study) $(STUDY_SHARED_LIBADD)
```

我们会看到这样的两行，里面就会去链接我们指定的`libuv.so`。

OK，我们来测试一下我们是否可以链接成功。我们在文件`study.cc`里面增加如下`PHP`接口：

```cpp
// 新增加的内容
#include <stdio.h>
#include <iostream>
#include <uv.h>

using namespace std;

uint64_t repeat = 0;

static void callback(uv_timer_t *handle)
{
    repeat = repeat + 1;
    cout << "repeat count:" << repeat << endl;
}

PHP_FUNCTION(study_timer_test)
{
		uv_loop_t *loop = uv_default_loop();
    uv_timer_t timer_req;
    uv_timer_init(loop, &timer_req);
    
    uv_timer_start(&timer_req, callback, 1000, 1000);
		uv_run(loop, UV_RUN_DEFAULT);
}
// 新增加的内容

// 省略了其他之前的代码

const zend_function_entry study_functions[] = {
	PHP_FE(study_coroutine_create, arginfo_study_coroutine_create)
	PHP_FALIAS(sgo, study_coroutine_create, arginfo_study_coroutine_create)
	PHP_FE(study_timer_test, NULL) // 新增加的一行
	PHP_FE_END
};
```

`OK`，重新编译、安装我们的`PHP`扩展：

```shell
~/codeDir/cppCode/study # make && make install
```

然后写下我们的测试脚本：

```php
<?php

study_timer_test();
```

运行：

```shell
~/codeDir/cppCode/study # php test.php 
repeat count:1
repeat count:2
repeat count:3
repeat count:4
repeat count:5
^C
~/codeDir/cppCode/study # 
```

我们发现，会有定时器的效果，每隔一秒就会去执行`callback`函数。

[下一篇：sleep（一）](./《PHP扩展开发》-协程-sleep（一）.md)