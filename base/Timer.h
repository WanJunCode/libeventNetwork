
#ifndef __CF_TIMER_H
#define __CF_TIMER_H

#include <event.h>
#include <functional>

class TimerManager;
class Timer{
    friend class TimerManager;
public:
    enum eTimerType { TIMER_ONCE, TIMER_PERSIST } ;

public:
    Timer(TimerManager *tmMgr);
    ~Timer();
public:
    void start(std::function<void()> task, unsigned interval, eTimerType ntype = TIMER_ONCE);
    void returnTimer();
//Override
public:
    void run();
    void reset();

protected:
    TimerManager *tmMgr_;
    std::function<void()> task_;
    eTimerType type_;
    unsigned interval_;
    struct event tm_;
    bool owned_;
};

#endif
