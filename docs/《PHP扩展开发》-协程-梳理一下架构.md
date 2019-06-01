# 梳理一下架构

[仓库地址](https://github.com/php-extension-research/study)

首先，我们需要去实现一个协程库，这个协程库是与`PHP`本身无关的。也就是说，我们实现的这个协程库可以用在其他地方，不一定是给`PHP`用的，功能主要是和上下文切换有关。我们把这个协程库放在目录`src/coroutine`里面。

然后，我们需要去实现`PHP`的协程，这个就和协程的一些行为有关了，包括`yield`、`resume`等等。我们把这些实现放在文件`study_coroutine.cc`、`study_coroutine.h`里面。

最后，我们需要提供协程的接口给`PHP`脚本来使用。我们把这些实现放在文件`study_coroutine_util.cc`里面。这个文件会对`PHP`调用协程接口时传递的参数做一些校验工作。

所以，我们创建以下目录以及文件：

```shell
src/coroutine（目录）
study_coroutine.h（文件）
study_coroutine.cc（文件）
study_coroutine_util.cc（文件）
```

我们在`study_coroutine.h`文件里面写下如下内容：

```c++
#ifndef STUDY_COROUTINE_H
#define STUDY_COROUTINE_H


#endif	/* STUDY_COROUTINE_H */
```

同时，我们需要修改我们的`config.m4`文件：

```shell
study_source_file="\
  study.cc \
  study_coroutine.cc \
  study_coroutine_util.cc \
  ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
  ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE}
"
```

[下一篇：协程创建（一）](《PHP扩展开发》-协程-协程创建（一）.md)

