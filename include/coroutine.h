#ifndef COROUTINE_H
#define COROUTINE_H

#include "context.h"

namespace Study
{
class Coroutine
{
    static long create(coroutine_func_t fn, void* args = nullptr);
};
}

#endif	/* COROUTINE_H */