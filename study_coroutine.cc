#include "study_coroutine.h"

using study::PHPCoroutine;
using study::Coroutine;

php_coro_task PHPCoroutine::main_task = {0};

long PHPCoroutine::create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv)
{
    php_coro_args php_coro_args;
    php_coro_args.fci_cache = fci_cache;
    php_coro_args.argv = argv;
    php_coro_args.argc = argc;
    save_task(get_task());
}

php_coro_task* PHPCoroutine::get_task()
{
    php_coro_task *task = (php_coro_task *) Coroutine::get_current_task();
    return task ? task : &main_task;
}

void PHPCoroutine::save_task(php_coro_task *task)
{
    save_vm_stack(task);
}

void PHPCoroutine::save_vm_stack(php_coro_task *task)
{
    task->vm_stack_top = EG(vm_stack_top);
    task->vm_stack_end = EG(vm_stack_end);
    task->vm_stack = EG(vm_stack);
    task->vm_stack_page_size = EG(vm_stack_page_size);
    task->execute_data = EG(current_execute_data);
}
