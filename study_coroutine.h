#ifndef STUDY_COROUTINE_H
#define STUDY_COROUTINE_H

#include "php_study.h"
#include "coroutine.h"

struct php_coro_args
{
    zend_fcall_info_cache *fci_cache;
    zval *argv;
    uint32_t argc;
};

// save the coroutine stack info
struct php_coro_task
{
    zval *vm_stack_top; // coroutine stack top
    zval *vm_stack_end; // coroutine stack end
    zend_vm_stack vm_stack; // current coroutine stack pointer
    size_t vm_stack_page_size;
    zend_execute_data *execute_data; // current coroutine stack frame
};

namespace Study
{
class PHPCoroutine
{
public:
    static long create(zend_fcall_info_cache *fci_cache, uint32_t argc, zval *argv);

protected:
    static php_coro_task main_task;

    static void save_task(php_coro_task *task);
    static void save_vm_stack(php_coro_task *task);
    static php_coro_task* get_task();
};
}

#endif	/* STUDY_COROUTINE_H */