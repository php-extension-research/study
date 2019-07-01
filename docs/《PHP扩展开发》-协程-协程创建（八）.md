# 协程创建（八）

这篇文章，我们来讲解一下

```cpp
long Coroutine::create(coroutine_func_t fn, void* args)
{
    return (new Coroutine(fn, args))->run();
}
```

中的

```cpp
(new Coroutine(fn, args))->run();
```

首先，我们创建了一个协程对象：

```cpp
new Coroutine(fn, args)
```

这是一个构造函数。实现如下，在`Study::Coroutine`类里面：

```cpp
protected:
		Coroutine(coroutine_func_t fn, void *private_data) :
            ctx(stack_size, fn, private_data)
    {
        cid = ++last_cid;
        coroutines[cid] = this;
    }
```

其中：

`fn`就是我们的`PHPCoroutine::create_func`，它完成了我们创建协程的基础工作，例如创建`PHP`栈帧、把传递给用户函数的参数放到栈帧上面、执行用户空间传递过来的函数（实际上就是去执行`zend_op_array`）。函数`PHPCoroutine::create_func`最终会被我们的协程入口函数调用（协程入口函数我们后面会去实现它）。

`private_data`是需要给协程跑的一些参数，实际上是`fn`在使用`private_data`。

然后，再传递这两个值去构造`ctx`，这个是创建出的这个协程的上下文，可以说是协程库最核心的地方了（这个上下文我们后面会讲）。其中，`stack_size`是协程栈的默认大小，我们定义在`src/coroutine/coroutine.cc`里面初始化的：

```cpp
size_t Coroutine::stack_size = DEFAULT_C_STACK_SIZE;
```

`DEFAULT_C_STACK_SIZE`这个宏我们在`include/coroutine.h`里面进行定义：

```cpp
#define DEFAULT_C_STACK_SIZE          (2 *1024 * 1024)
```

大小是`2M`。

然后，执行：

```cpp
cid = ++last_cid;
coroutines[cid] = this;
```

其中：

`cid`是用来记录创建出的这个协程的`id`。我们在`Study::Coroutine`这个类里面进行声明：

```cpp
protected:
    long cid;
```

`last_cid`是用来记录创建的最后一个协程的`cid`，我们也在`Study::Coroutine`这个类里面进行声明：

```cpp
protected:
		static long last_cid;
```

然后，我们在`src/coroutine/coroutine.cc`里面进行初始化：

```cpp
long Coroutine::last_cid = 0;
```

所以，我们创建的第一个协程它的id是1。

OK，我们现在完成了创建协程的工作。

接下来，我们开始实现最核心的东西--**协程的上下文**。

我们新建两个文件`include/context.h`和`src/coroutine/context.cc`。（很熟悉的，我们需要修改我们的`config.m4`文件，新增`src/coroutine/context.cc`，这里不再多说了）

`context.h`的内容如下：

```cpp
#ifndef CONTEXT_H
#define CONTEXT_H

#include "asm_context.h"

typedef fcontext_t coroutine_context_t;
typedef void (*coroutine_func_t)(void*);

namespace Study
{
class Context
{
public:
    Context(size_t stack_size, coroutine_func_t fn, void* private_data);

protected:
    coroutine_func_t fn_;
    uint32_t stack_size_;
    void *private_data_;
};
}

#endif	/* CONTEXT_H */
```

我们现在来实现一下`Study::Context`的构造函数。我们在`src/coroutine/context.cc`中进行实现：

```cpp
#include "context.h"
#include "study.h"

using namespace Study;

Context::Context(size_t stack_size, coroutine_func_t fn, void* private_data) :
        fn_(fn), stack_size_(stack_size), private_data_(private_data)
{
    swap_ctx_ = nullptr;

    stack_ = (char*) malloc(stack_size_);
    
    void* sp = (void*) ((char*) stack_ + stack_size_);
    ctx_ = make_fcontext(sp, stack_size_, (void (*)(intptr_t))&context_func);
}
```

所以，我们在

```cpp
Coroutine(coroutine_func_t fn, void *private_data) :
            ctx(stack_size, fn, private_data)
```

这个构造函数中传递的这三个值就会赋值给`Study::Context`的`fn_`、`stack_size_`、`private_data_`。

```cpp
stack_ = (char*) malloc(stack_size_);
```

是创建一个`C`栈（实际上是从堆中分配的内存）。

```cpp
void* sp = (void*) ((char*) stack_ + stack_size_);
```

这行代码是把堆模拟成栈的行为。与之前`PHP`栈的操作类似。

```cpp
ctx_ = make_fcontext(sp, stack_size_, (void (*)(intptr_t))&context_func);
```

这行代码是设置这个最底层的协程的上下文`ctx_`，比如栈地址，栈大小，协程的入口函数`context_func`。而`make_fcontext`这个设置上下文的函数式用的`boost.asm`里面的库。我们需要声明这是一个外部函数，我们创建文件`include/asm_context.h`，然后在里面进行声明：

```cpp
#ifndef ASM_CONTEXT_H
#define ASM_CONTEXT_H

#include "study.h"

typedef void* fcontext_t;

extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(intptr_t)) asm("make_fcontext");

#endif	/* ASM_CONTEXT_H */
```

我们在`Study::Context`中定义`ctx_`这个成员变量：

```cpp
protected:
    coroutine_context_t ctx_;
```

协程的入口函数我们先在`Study::Context`中进行定义：

```cpp
public:
    static void context_func(void* arg); // coroutine entry function
```

然后在`src/coroutine/context.cc`中进行实现：

```cpp
void Context::context_func(void *arg)
{
    Context *_this = (Context *) arg;
    _this->fn_(_this->private_data_);
    _this->swap_out();
}
```

可以看到，这段代码就是去调用`fn_`，也就是`PHPCoroutine::create_func`，并且给它传递参数`private_data_`，也就是`php_coro_args *`。

`swap_out`的作用是让出当前协程的上下文，去加载其他协程的上下文。就是当我们跑完了这个协程，需要恢复其他的协程的上下文，让其他的协程继续运行。我们先在`include/context.h`里面进行定义：

```cpp
public:
    bool swap_out();
```

然后在`src/coroutine/context.cc`里面去实现：

```cpp
bool Context::swap_out()
{
    jump_fcontext(&ctx_, swap_ctx_, (intptr_t) this, true);
    return true;
}
```

`jump_fcontext`这个函数也是`boost.asm`库提供的，我们需要在`include/asm_context.h`里面进行声明：

```cpp
extern intptr_t jump_fcontext(fcontext_t *ofc, fcontext_t nfc, intptr_t vp, bool preserve_fpu = false) asm("jump_fcontext");
```

这个函数的作用是保存当前的上下文到`ofc`，然后加载`nfc`这个上下文。

OK，到现在，我们实现了最底层的协程上下文的初始化工作。（注意，`make_fcontext`只是去设置协程的上下文，并不会去执行）

最后，我们就需要让这个底层协程运行起来，一旦它运行起来了，那么我们的`Context::context_func`函数才会被调用，最后我们用户空间的函数才会被调用。所以，我们现在去实现`Study::Coroutine::run()`这个方法，在`include/coroutine.h`里面：

```cpp
protected:
    long run()
    {
        long cid = this->cid;
        origin = current;
        current = this;
        ctx.swap_in();
        return cid;
    }
```

其中：

```cpp
origin = current;
current = this;
```

因为我们需要跑一个新的协程，所以当前正在跑的协程是`origin`的，这个刚被创建的协程才是`current`，是现在需要被执行的协程。`origin`我们在`Study::Coroutine`中进行定义：

```cpp
protected:
    Coroutine *origin;
```

然后，我们调用了`ctx.swap_in()`，这个函数的作用和`ctx.swap_out`刚好相反。`ctx.swap_in()`的作用是加载上下文`ctx`。所以，当我们执行`run`函数的时候，这个协程的上下文`ctx`就会被加载，所以这个最底层的协程就会被运行，所以我们的`PHP`协程就会被运行。

我们先在`include/context.h`里面声明`swap_in()`：

```cpp
public:
    bool swap_in();
```

然后在`src/coroutine/context.cc`里面去实现：

```cpp
bool Context::swap_in()
{
    jump_fcontext(&swap_ctx_, ctx_, (intptr_t) this, true);
    return true;
}
```

OK，到这里，我们总算是实现了协程的创建与运行。现在，我们来编译、安装这个扩展：

```shell
~/codeDir/cppCode/study # phpize --clean
~/codeDir/cppCode/study # phpize
~/codeDir/cppCode/study # ./configure
~/codeDir/cppCode/study # make clean && make && make install
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
```

OK，编译、安装成功。

我们编写我们的`PHP`脚本：

```php
<?php

function task($n, $a, $b)
{
	if ($n === 1) {
		echo "coroutine[1] create successfully" . PHP_EOL;
	}
	if ($n === 2) {
		echo "coroutine[2] create successfully" . PHP_EOL;
	}
	
	echo $a . PHP_EOL;
	echo $b . PHP_EOL;
}

echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 1, 'a', 'b');
echo "main coroutine" . PHP_EOL;
Study\Coroutine::create('task', 2, 'c', 'd');
```

执行结果：

```shell
~/codeDir/cppCode/study # php test.php 
main coroutine
coroutine[1] create successfully
a
b
main coroutine
coroutine[2] create successfully
c
d
~/codeDir/cppCode/study # 
```

符合预期。















