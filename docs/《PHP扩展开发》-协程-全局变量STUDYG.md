# 全局变量STUDYG

我们在上一节里面说到了需要使用`epoll`来实现协程的调度，实际上`epoll`也可以实现我们的`sleep`功能，我们现在来实现一下。首先，我们需要一个全局的`epoll`，我把我们今后会用到的全局变量保存在统一的一个结构体里面。我们在文件`study.h`里面定义这个结构体：

```cpp
typedef struct
{
    stPoll_t poll;
} stGlobal_t;
```

目前，里面存放了`stPoll_t`结构体，这个结构体我们也在`study.h`里面进行定义：

```cpp
typedef struct
{
    int epollfd;
    int ncap;
    struct epoll_event *events;
} stPoll_t;
```

因为我们使用了`epoll_event`，所以，我们需要在`study.h`里面引入头文件：

```cpp
#include <sys/epoll.h>
```

其中，

`epollfd`用来保存我们创建的那个`epollfd`。

`events`是用来保存`epoll`返回的事件。

`ncap`是`events`数组的大小。

然后，我们新建一个文件`src/core/base.cc`，文件内容如下：

```cpp
#include "study.h"

stGlobal_t StudyG;
```

然后，我们需要在文件`study.h`里面去声明这个变量在其他地方定义了：

```cpp
extern stGlobal_t StudyG;
```

为什么我们不直接在文件`study.h`里面定义全局变量`StudyG`呢？因为，如果我们在文件`study.h`里面定义了这个全局的变量，那么，如果头文件`study.h`被多个地方引入了，那么，编译器就会认为这个全局变量重复定义了，所以，我们需要在一个地方去定义它，然后在另一个地方声明这是一个外部变量。

因为我们增加了一个需要被编译到的源文件，所以我们需要修改我们的`config.m4`文件：

```shell
study_source_file="\
    study.cc \
    study_coroutine.cc \
    study_coroutine_util.cc \
    src/coroutine/coroutine.cc \
    src/coroutine/context.cc \
    ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
    ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE} \
    study_server_coro.cc \
    src/socket.cc \
    src/log.cc \
    src/error.cc \
    src/core/base.cc
"
```

然后，我们需要去修改我们的调度器代码：

```cpp
int PHPCoroutine::scheduler()
{
    int timeout;
    size_t size;
    uv_loop_t *loop = uv_default_loop();

    StudyG.poll.epollfd = epoll_create(256);
    StudyG.poll.ncap = 16;
    size = sizeof(struct epoll_event) * StudyG.poll.ncap;
    StudyG.poll.events = (struct epoll_event *) malloc(size);
    memset(StudyG.poll.events, 0, size);

    while (loop->stop_flag == 0)
    {
        timeout = uv__next_timeout(loop);
        epoll_wait(StudyG.poll.epollfd, StudyG.poll.events, StudyG.poll.ncap, timeout);

        loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
        uv__run_timers(loop);

        if (uv__next_timeout(loop) < 0)
        {
            uv_stop(loop);
        }
    }

    return 0;
}
```

代码很简单，主要过程如下：

```
1、创建一个epollfd，然后保存在全局变量
2、在堆中为全局的StudyG.poll.events分配内存，默认的大小是16
3、把usleep替换为epoll_wait，时间是timeout
```

然后，我们需要重新生成一下我们的`Makefile`：

```shell
~/codeDir/cppCode/study # phpize --clean ; phpize ; ./configure
```

然后编译、安装扩展：

```shell
~/codeDir/cppCode/study # make ; make install
```

编写测试代码：

```php
<?php

Sgo(function ()
{
    var_dump("here1");
    Sco::sleep(0.5);
    var_dump("here2");
    Sco::sleep(1);
    var_dump("here3");
    Sco::sleep(1.5);
    var_dump("here4");
    Sco::sleep(2);
    var_dump("here5");
});

Sco::scheduler();
```

执行脚本：

```shell
~/codeDir/cppCode/study # php test.php 
string(5) "here1"
string(5) "here2"
string(5) "here3"
string(5) "here4"
string(5) "here5"
~/codeDir/cppCode/study # 
```

符合预期。