#ifndef COROUTINE_H
#define COROUTINE_H

#include "context.h"

namespace Study
{
class Coroutine
{
public:
    static void* get_current_task();
    static long create(coroutine_func_t fn, void* args = nullptr);
    void* get_task();

protected:
    static Coroutine* current;
    void *task = nullptr;
};
}

#endif	/* COROUTINE_H */