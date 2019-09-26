#ifndef COROUTINE_CHANNEL_H
#define COROUTINE_CHANNEL_H

#include "study.h"
#include "coroutine.h"

namespace study { namespace coroutine {
class Channel
{
public:
    enum opcode
    {
        PUSH = 1,
        POP = 2,
    };

    Channel(size_t _capacity = 1);
    ~Channel();
    void* pop(double timeout = -1);
    bool push(void *data, double timeout = -1);
    bool empty();
    void* pop_data();

protected:
    size_t capacity = 1;
    bool closed = false;
    std::queue<Coroutine*> producer_queue;
    std::queue<Coroutine*> consumer_queue;
    std::queue<void*> data_queue;
};
}
}

#endif	/* COROUTINE_CHANNEL_H */ 