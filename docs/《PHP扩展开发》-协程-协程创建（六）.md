# 协程创建（六）

这篇文章，我们开始实现`php_coro_task* PHPCoroutine::get_task()`。

首先，我们认为有一个主协程，这是其他所有协程的开始。所以，我们在`Study::PHPCoroutine`中定义一个成员变量`main_task`，我希望它是`protected`的。在文件`study_coroutine.h`中定义：

```cpp
protected:
    static php_coro_task main_task;
```

然后，我们本来是可以这么实现`get_task()`的：

```cpp
php_coro_task PHPCoroutine::main_task = {0};

php_coro_task* PHPCoroutine::get_task()
{
    php_coro_task *task = PHPCoroutine::get_current_task();
    return task ? task : &main_task;
}
```

但是，我们希望这套协程库不仅仅适用于`PHP`，我们还希望以后可以适用于其他地方。所以我们获取当前协程的功能放在更加低层次的`Study::Coroutine`里面来实现它。

所以，我们应该这么实现`php_coro_task* PHPCoroutine::get_task()`：

```cpp
php_coro_task PHPCoroutine::main_task = {0};

php_coro_task* PHPCoroutine::get_task()
{
    php_coro_task *task = (php_coro_task *) Coroutine::get_current_task();
    return task ? task : &main_task;
}
```

我们让`Coroutine::get_current_task()`返回一个`void`类型的指针，然后，根据我们上层需要的协程结构进行转换即可，这样我们的协程库就可以在多个地方使用了。所以，我们需要去实现`Study::Coroutine::get_current_task`。

很显然，这个方法应该是`public`的，因为我们需要在`PHPCoroutine`这个类里面去调用它，所以我们做如下的声明，在文件`include/coroutine.h`的`Study::Coroutine`里面：

```cpp
public:
    static void* get_current_task();
```

然后，我们再在`Study::Coroutine`类里面定义一个`Study::Coroutine`类型的指针：

```cpp
protected:
    static Coroutine* current;
```

然后，我们在`src/coroutine`目录下创建一个文件`coroutine.cc`，然后在这个文件里面来实现`void * Coroutine::get_current_task`这个方法：

```cpp
#include "coroutine.h"

using Study::Coroutine;

Coroutine* Coroutine::current = nullptr;

void* Coroutine::get_current_task()
{
    return Coroutine::current ? Coroutine::current->get_task() : nullptr;
}
```

因为我们新创建了一个文件`coroutine.cc`，所以我们需要修改我们的`config.m4`文件：

```shell
study_source_file="\
    study.cc \
    study_coroutine.cc \
    study_coroutine_util.cc \
    src/coroutine/coroutine.cc \
    ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
    ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE}
"
```

新增了`src/coroutine/coroutine.cc`。

最后，我们只需要实现一下`Study::Coroutine::get_task`即可，我们在`include/coroutine.h`的`Study::Coroutine`中进行声明：

```cpp
public:
    void* get_task();
```

然后在`src/coroutine/coroutine.cc`中进行实现：

```cpp
void* Coroutine::get_task()
{
    return task;
}
```

我们需要在`include/coroutine.h`的`Study::Coroutine`类里面去定义一下这个`task`成员变量：

```cpp
protected:
    void *task = nullptr;
```

这样，我们就实现了保存`PHP`协程栈的功能。