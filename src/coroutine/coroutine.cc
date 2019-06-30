#include "coroutine.h"

using Study::Coroutine;

Coroutine* Coroutine::current = nullptr;

void* Coroutine::get_current_task()
{
    return current ? current->get_task() : nullptr;
}

void* Coroutine::get_task()
{
    return task;
}