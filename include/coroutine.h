#ifndef COROUTINE_H
#define COROUTINE_H

#include "context.h"
#include <unordered_map>

#define DEFAULT_C_STACK_SIZE          (2 *1024 * 1024)

namespace study
{
class Coroutine
{
public:
    static std::unordered_map<long, Coroutine*> coroutines;

    static void* get_current_task();
    static long create(coroutine_func_t fn, void* args = nullptr);
    void* get_task();
    static Coroutine* get_current();
    void set_task(void *_task);
    void yield();
    void resume();
    static int sleep(double seconds);

    inline long get_cid()
    {
        return cid;
    }

    static inline Coroutine* get_by_cid(long cid)
    {
        auto i = coroutines.find(cid);
        return i != coroutines.end() ? i->second : nullptr;
    }

protected:
    Coroutine *origin;
    static Coroutine* current;
    void *task = nullptr;
    static size_t stack_size;
    Context ctx;
    long cid;
    static long last_cid;

    Coroutine(coroutine_func_t fn, void *private_data) :
            ctx(stack_size, fn, private_data)
    {
        cid = ++last_cid;
        coroutines[cid] = this;
    }

    long run()
    {
        long cid = this->cid;
        origin = current;
        current = this;
        ctx.swap_in();
        if (ctx.is_end())
        {
            current = origin;
            coroutines.erase(cid);
            delete this;
        }
        return cid;
    }
};
}

#endif	/* COROUTINE_H */