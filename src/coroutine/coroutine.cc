#include "coroutine.h"

using study::Coroutine;

void* Coroutine::get_current_task()
{
    return Coroutine::current ? Coroutine::current->get_task() : nullptr;
}

void* Coroutine::get_task()
{
    return task;
}