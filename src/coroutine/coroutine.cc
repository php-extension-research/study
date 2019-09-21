#include "coroutine.h"
#include "timer.h"

using study::Coroutine;

Coroutine* Coroutine::current = nullptr;
long Coroutine::last_cid = 0;
std::unordered_map<long, Coroutine*> Coroutine::coroutines;
size_t Coroutine::stack_size = DEFAULT_C_STACK_SIZE;
st_coro_on_swap_t Coroutine::on_yield = nullptr;
st_coro_on_swap_t Coroutine::on_resume = nullptr;

void* Coroutine::get_current_task()
{
    return current ? current->get_task() : nullptr;
}

void* Coroutine::get_task()
{
    return task;
}

Coroutine* Coroutine::get_current()
{
    return current;
}

void Coroutine::set_task(void *_task)
{
    task = _task;
}

long Coroutine::create(coroutine_func_t fn, void* args)
{
    return (new Coroutine(fn, args))->run();
}

void Coroutine::yield()
{
    assert(current == this);
    on_yield(task);
    current = origin;
    ctx.swap_out();
}

void Coroutine::resume()
{
    assert(current != this);
    on_resume(task);
    origin = current;
    current = this;
    ctx.swap_in();
    if (ctx.is_end())
    {
        current = origin;
        coroutines.erase(cid);
        delete this;
    }
}

static void sleep_timeout(void *param)
{
    ((Coroutine *) param)->resume();
}

int Coroutine::sleep(double seconds)
{
    Coroutine* co = Coroutine::get_current();

    timer_manager.add_timer(seconds * Timer::SECOND, sleep_timeout, (void*)co);
   
    co->yield();
    return 0;
}

void Coroutine::set_on_yield(st_coro_on_swap_t func)
{
    on_yield = func;
}

void Coroutine::set_on_resume(st_coro_on_swap_t func)
{
    on_resume = func;
}