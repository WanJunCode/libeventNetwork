// 使用 shared_ptr 管理 timer
// grab 时从 stack 中取出,如果没有则 new ,取出时加入 vector 中

// vector 保存所有已取出定时器的 shared_ptr
// stack 中保存可以重复使用的 timer

// 定时器使用完后 要么删除在 vector 中的 shared_ptr ,或者回收到stack 中
// timer 不能使用 shared_ptr 管理

// 解决dispatch延时的办法,在base上注册一个 event,然后在 timer manager->stop 中触发回调,在dispatch线程中breakloop

#ifndef __CF_TIMRE_MANAGER_H
#define __CF_TIMRE_MANAGER_H

#include <vector>
#include <stack>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "Timer.h"
#include <memory>

#define TIMER_STACK_SIZE 128

class TimerManager:public std::enable_shared_from_this<TimerManager>{
public:
    // static TimerManager& getInstance() {
    //     static TimerManager instance;
    //     return instance;
    // }
    typedef std::shared_ptr<Timer> Timer_ptr;
    typedef std::pair<Timer *,struct event *> timer_event;

    TimerManager();
    ~TimerManager();
public:
    inline void timerMaxSize(int limit = TIMER_STACK_SIZE) {
        timerStackLimit_ = limit;
    };
    void init();
    Timer *grabTimer();
    void stop();
public:
    // 线程执行函数
    void runInThread();
    void addTimer(Timer *timer);
    void returnTimer(Timer *timer); // timer use this 将自己回收或者删除

    static void timeoutCallback(int fd, short event, void *arg);
    static void eventBreakLoop(int fd, short event, void *arg);
private:
    void removeEvent(Timer *timer);

private:
    friend class Timer;
    int breakloop[2];
    struct event *ev_breakloop;
    struct event_base *base;
    std::vector<timer_event> eventVec_;
    std::mutex mutex_;
    std::atomic<bool> bexit_;
    std::condition_variable expired_cond_;
    std::vector<Timer *> vtimer_;
    std::stack<Timer *> timerStack_;
    size_t timerStackLimit_;
};

#endif