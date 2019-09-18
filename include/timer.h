#ifndef TIMER_H
#define TIMER_H

#include "study.h"

typedef void (*timer_func_t)(void*);

namespace study
{
class TimerManager;

class Timer
{
friend class TimerManager;
friend class CompareTimerPointer;
public:
    static const uint64_t MILLI_SECOND;
    static const uint64_t SECOND;
    Timer(uint64_t _timeout, timer_func_t _callback, void *_private_data, TimerManager *_timer_manager);
    static uint64_t get_current_ms();
    
private:
    uint64_t timeout = 0;
    uint64_t exec_msec = 0;
    timer_func_t callback;
    void *private_data;
    TimerManager *timer_manager = nullptr;
};

class CompareTimerPointer
{
public:
    bool operator () (Timer* &timer1, Timer* &timer2) const
    {
        return timer1->exec_msec > timer2->exec_msec;
    }
};

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();
    void add_timer(int64_t _timeout, timer_func_t _callback, void *_private_data);
    int64_t get_next_timeout();
    void run_timers();
private:
    std::priority_queue<Timer*, std::vector<Timer*>, CompareTimerPointer> timers;
};

extern TimerManager timer_manager;
}

#endif	/* TIMER_H */
