# 协程创建（七）

我们在上篇文章，成功的保存了主协程的上下文信息，现在，我们就需要为我们的任务函数创建协程了。

我们在`PHPCoroutine::create`中写入：

```cpp
long PHPCoroutine::create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv)
{
    php_coro_args php_coro_args;
    php_coro_args.fci_cache = fci_cache;
    php_coro_args.argv = argv;
    php_coro_args.argc = argc;
    save_task(get_task());

    return Coroutine::create(create_func, (void*) &php_coro_args);
}
```

其中，`PHPCoroutine::create_func`是用来创建协程任务的，我们可以了解为这是一个辅助函数，辅助我们去创建协程。它是：

```cpp
typedef void(* coroutine_func_t)(void *) 
```

类型的函数指针。

而`php_coro_args`则是传递给`create_func`的参数。而这些参数里面，就包括了我们需要去执行的用户函数以及传递给这个用户函数的参数信息。

OK，我们现在来实现一下`PHPCoroutine::create_func`。我们先在`Study::PHPCoroutine`类中声明一下这个方法：

```cpp
protected:
    static void create_func(void *arg);
```

然后，我们在文件`study_coroutine.cc`中来实现这个函数：

```cpp
void PHPCoroutine::create_func(void *arg)
{
    int i;
    php_coro_args *php_arg = (php_coro_args *) arg;
    zend_fcall_info_cache fci_cache = *php_arg->fci_cache;
    zend_function *func = fci_cache.function_handler;
    zval *argv = php_arg->argv;
    int argc = php_arg->argc;
    php_coro_task *task;
    zend_execute_data *call;
    zval _retval, *retval = &_retval;

    vm_stack_init(); // get a new php stack
    call = (zend_execute_data *) (EG(vm_stack_top));
    task = (php_coro_task *) EG(vm_stack_top);
    EG(vm_stack_top) = (zval *) ((char *) call + PHP_CORO_TASK_SLOT * sizeof(zval));

    call = zend_vm_stack_push_call_frame(
        ZEND_CALL_TOP_FUNCTION | ZEND_CALL_ALLOCATED,
        func, argc, fci_cache.called_scope, fci_cache.object
    );

    for (i = 0; i < argc; ++i)
    {
        zval *param;
        zval *arg = &argv[i];
        param = ZEND_CALL_ARG(call, i + 1);
        ZVAL_COPY(param, arg);
    }

    call->symbol_table = NULL;

    EG(current_execute_data) = call;

    save_vm_stack(task);

    task->co = Coroutine::get_current();
    task->co->set_task((void *) task);

    if (func->type == ZEND_USER_FUNCTION)
    {
        ZVAL_UNDEF(retval);
        EG(current_execute_data) = NULL;
        zend_init_func_execute_data(call, &func->op_array, retval);
        zend_execute_ex(EG(current_execute_data));
    }

    zval_ptr_dtor(retval);
}
```

代码很长，我们慢慢来分析。

```cpp
int i;
php_coro_args *php_arg = (php_coro_args *) arg;
zend_fcall_info_cache fci_cache = *php_arg->fci_cache;
zend_function *func = fci_cache.function_handler;
zval *argv = php_arg->argv;
int argc = php_arg->argc;
php_coro_task *task;
zend_execute_data *call;
```

这一段代码只是简单的把一些核心内容提取出来，存放在其他变量里面。这一段代码纯粹是为了增强代码的可读性。

```cpp
vm_stack_init();
```

这个方法的目的是初始化一个新的`PHP`栈，因为我们即将要创建一个协程了。我们来实现一下这个方法。

首先，我们需要在`Study::PHPCoroutine`类里面来声明一下：

```cpp
protected:
    static void vm_stack_init(void);
```

然后，我们在文件`study_coroutine.cc`里面实现它：

```cpp
void PHPCoroutine::vm_stack_init(void)
{
    uint32_t size = DEFAULT_PHP_STACK_PAGE_SIZE;
    zend_vm_stack page = (zend_vm_stack) emalloc(size);

    page->top = ZEND_VM_STACK_ELEMENTS(page);
    page->end = (zval*) ((char*) page + size);
    page->prev = NULL;

    EG(vm_stack) = page;
    EG(vm_stack)->top++;
    EG(vm_stack_top) = EG(vm_stack)->top;
    EG(vm_stack_end) = EG(vm_stack)->end;
    EG(vm_stack_page_size) = size;
}
```

首先，我们把我们定义好的默认`PHP`栈一页的大小赋值给`size`，我们在文件`study_coroutine.h`里面来进行声明：

```cpp
#define DEFAULT_PHP_STACK_PAGE_SIZE       8192
```

然后

```cpp
zend_vm_stack page = (zend_vm_stack) emalloc(size);
```

的作用是从堆上面分配出`size`的大小的空间，然后把地址赋值给`zend_vm_stack`。

```cpp
page->top = ZEND_VM_STACK_ELEMENTS(page);
page->end = (zval*) ((char*) page + size);
page->prev = NULL;
```

这段代码的作用是把我们的堆模拟成栈的行为。因为按照`cpp`的内存模型，栈的地址空间一般是由高地址往低地址增长的。

```cpp
EG(vm_stack) = page;
EG(vm_stack)->top++;
EG(vm_stack_top) = EG(vm_stack)->top;
EG(vm_stack_end) = EG(vm_stack)->end;
EG(vm_stack_page_size) = size;
```

这段代码的作用是去修改现在的`PHP`栈，让它指向我们申请出来的新的`PHP`栈空间。

OK，初始化一个新的`PHP`栈分析完毕。

我们继续分析`PHPCoroutine::create_func`剩余的代码：

```cpp
call = zend_vm_stack_push_call_frame(
    ZEND_CALL_TOP_FUNCTION | ZEND_CALL_ALLOCATED,
    func, argc, fci_cache.called_scope, fci_cache.object
);
```

这段代码的作用是分配一个`zend_execute_data`，分配一块用于当前作用域的内存空间。因为用户空间的函数会被编译成`zend_op_array`，`zend_op_array`是在`zend_execute_data`上执行的。而我们上面的`fci_cache.function_handler`是一个`zend_function`，这个`zend_function`是一个`union`，里面包含了一些函数的公共信息以及具体的函数类型，用户定义的函数或者内部函数（内部函数指的是`PHP`内置的函数或者`C`扩展提供的函数）：

```cpp
union _zend_function {
	zend_uchar type;	/* MUST be the first element of this struct! */
	uint32_t   quick_arg_flags;

	struct {
		zend_uchar type;  /* never used */
		zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
		uint32_t fn_flags;
		zend_string *function_name;
		zend_class_entry *scope;
		union _zend_function *prototype;
		uint32_t num_args;
		uint32_t required_num_args;
		zend_arg_info *arg_info;
	} common;

	zend_op_array op_array;
	zend_internal_function internal_function;
};
```

我们现在要用到的就是`zend_function`里面的：

```cpp
zend_op_array op_array
```

因为，我们目前只打算支持用户自定义的函数协程化。并且，我们现在不打算支持类对象方法的协程化。并且我们只打算支持`PHP7.3.5`，并且不打算支持传递引用类型的参数。

```cpp
for (i = 0; i < argc; ++i)
{
  zval *param;
  zval *arg = &argv[i];
  param = ZEND_CALL_ARG(call, i + 1);
  ZVAL_COPY(param, arg);
}
```

这段代码的作用是用来逐个获取`zend_execute_data`上第`i`个参数应该在的`zval`地址，然后把地址值赋值给`param`，获取到了参数地址之后，我们就可以把我们传递给用户函数的参数一个一个的拷贝到`zend_execute_data`上面去。

```cpp
EG(current_execute_data) = call;
```

初始化`call`这个`zend_execute_data`之后，我们把它赋值给`EG(current_execute_data)`。

```cpp
save_vm_stack(task);
task->co = Coroutine::get_current();
task->co->set_task((void *) task);
```

把当前的协程栈信息保存在`task`里面。

OK，实现完了`PHPCoroutine::create_func`之后，我们接着实现`Study::Coroutine::create`这个方法：

```cpp
long Coroutine::create(coroutine_func_t fn, void* args)
{
    return (new Coroutine(fn, args))->run();
}
```

这个方法很简单，就是创建一个协程，然后让它运行。限于篇幅原因以及上班工作时间到了，我将会开另一篇文章来讲解。











