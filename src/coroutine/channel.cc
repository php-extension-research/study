#include "coroutine.h"
#include "coroutine_channel.h"
#include "timer.h"

using study::Coroutine;
using study::coroutine::Channel;
using study::Timer;
using study::TimerManager;
using study::timer_manager;

Channel::Channel(size_t _capacity):
    capacity(_capacity)
{
}

Channel::~Channel()
{
}

static void sleep_timeout(void *param)
{
    ((Coroutine *) param)->resume();
}

void* Channel::pop(double timeout)
{
    Coroutine *co = Coroutine::get_current();
    void *data;

    if (data_queue.empty())
    {
        if (timeout > 0)
        {
            timer_manager.add_timer(timeout * Timer::SECOND, sleep_timeout, (void*)co);
        }
        consumer_queue.push(co);
        co->yield();
    }

    if (data_queue.empty())
    {
        return nullptr;
    }

    data = data_queue.front();
    data_queue.pop();

    /**
     * notice producer
     */
    if (!producer_queue.empty())
    {
        co = producer_queue.front();
        producer_queue.pop();
        co->resume();
    }

    return data;
}

bool Channel::push(void *data, double timeout)
{
    Coroutine *co = Coroutine::get_current();
    if (data_queue.size() == capacity)
    {
        if (timeout > 0)
        {
            timer_manager.add_timer(timeout * Timer::SECOND, sleep_timeout, (void*)co);
        }
        producer_queue.push(co);
        co->yield();
    }

    /**
     * channel full
     */
    if (data_queue.size() == capacity)
    {
        return false;
    }

    data_queue.push(data);

    /**
     * notice consumer
     */
    if (!consumer_queue.empty())
    {
        co = consumer_queue.front();
        consumer_queue.pop();
        co->resume();
    }

    return true;
}

bool Channel::empty()
{
    return data_queue.empty();
}

void* Channel::pop_data()
{
    void *data;

    if (data_queue.size() == 0)
    {
        return nullptr;
    }
    data = data_queue.front();
    data_queue.pop();
    return data;
}