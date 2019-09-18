#include "context.h"
#include "study.h"
#include "log.h"
#include <iostream>

using namespace study;
using namespace std;

Context::Context(size_t stack_size, coroutine_func_t fn, void* private_data) :
        fn_(fn), stack_size_(stack_size), private_data_(private_data)
{
    swap_ctx_ = nullptr;

    try
    {
        stack_ = new char[stack_size_];
    }
    catch(const std::bad_alloc& e)
    {
        stError("%s", e.what());
    }

    void* sp = (void*) ((char*) stack_ + stack_size_);
    ctx_ = make_fcontext(sp, stack_size_, (void (*)(intptr_t))&context_func);
}

Context::~Context()
{
    if (swap_ctx_)
    {
        delete[] stack_;
        stack_ = nullptr;
    }
}

bool Context::swap_in()
{
    jump_fcontext(&swap_ctx_, ctx_, (intptr_t) this, true);
    return true;
}

bool Context::swap_out()
{
    jump_fcontext(&ctx_, swap_ctx_, (intptr_t) this, true);
    return true;
}

void Context::context_func(void *arg)
{
    Context *_this = (Context *) arg;
    _this->fn_(_this->private_data_);
    _this->end_ = true;
    _this->swap_out();
}