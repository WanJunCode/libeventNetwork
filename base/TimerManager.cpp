#include "TimerManager.h"
#include <algorithm>
#include "../main/logcpp.h"

#define evtimer2_set(ev, cb, arg) event_set(ev, -1, EV_PERSIST, cb, arg)

TimerManager::TimerManager()
    : bexit_(false)
    , timerStackLimit_(TIMER_STACK_SIZE) {
    base = event_base_new();
    if(base == NULL){
        LOG_FATAL("timer manager event base new failure\n");
    }

}

TimerManager::~TimerManager() {
    if(bexit_ == false){
        stop();
    }
    LOG_DEBUG("timer manager des\n");
}

void TimerManager::init(){
    // 共享指针存入栈中
    for (int i = 0; i < 10; ++i) {
        timerStack_.push(std::make_shared<Timer>(shared_from_this()));
    }
    LOG_DEBUG("timer manager init\n");
}
void TimerManager::stop(){
    bexit_ = true;
    std::lock_guard<std::mutex> locker(mutex_);
    while (vtimer_.size()) {
        std::shared_ptr<Timer> ref = vtimer_.back();
        // 结束在 event 上注册的定时器
        ref->reset();
        removeEvent(ref.get());
        vtimer_.pop_back();
    }
    event_base_loopbreak(base);
    LOG_DEBUG("timer manager stop event loop break\n");
    expired_cond_.notify_one();
    event_base_free(base);
}

// static
void TimerManager::timeoutCallback(int fd, short event, void *args) {
    LOG_DEBUG("TimerManager::timeoutCallback 回调\n");
    Timer *tmProc = (Timer *)args;
    if (tmProc) {
        tmProc->run();
        if (Timer::TIMER_ONCE == tmProc->type_) {
            tmProc->returnTimer();
        }
    }
}

std::shared_ptr<Timer> TimerManager::grabTimer(){
    std::shared_ptr<Timer> timer = nullptr;
    if (timerStack_.empty()) {
        timer = std::make_shared<Timer>(shared_from_this());
        vtimer_.push_back(timer);
    } else {
        std::lock_guard<std::mutex> locker(mutex_);
        if (nullptr != (timer = timerStack_.top())) {
            // 取出之前加入vector
            vtimer_.push_back(timer);
            timerStack_.pop();
        }
    }
    return timer;
}

// 将 timer 的事件添加到 base 中
void TimerManager::addTimer(Timer *timer) {
    {
        struct event *timer_ev;
        /* Initalize one event */
        if (Timer::TIMER_ONCE == timer->type_) {
            LOG_DEBUG("添加一次性时间器\n");
            timer_ev = event_new(base, -1, EV_READ,TimerManager::timeoutCallback, timer);
        } else {
            timer_ev = event_new(base, -1, EV_PERSIST,TimerManager::timeoutCallback, timer);
        }

        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = (int)(timer->interval_);
        tv.tv_usec = (timer->interval_ - tv.tv_sec) * 1000000;
        LOG_DEBUG("tv.tv_sec = [%d]\n",tv.tv_sec);

        event_add(timer_ev, &tv);
        // 将 timer 和 event 的关系保存
        eventVec_.push_back( std::pair<Timer *,struct event *>(timer,timer_ev));
    }

    // 告诉dispatch线程开始监听事件
    expired_cond_.notify_one();
}

void TimerManager::returnTimer(Timer *timer) {
    std::lock_guard<std::mutex> locker(mutex_);
    for(size_t idx = 0;idx<vtimer_.size();++idx){
        if(vtimer_[idx].get()==timer){
            LOG_DEBUG("idx = [%d] is timer\n",idx);
            if(timerStack_.size()<TIMER_STACK_SIZE){
                timerStack_.push(vtimer_[idx]);
                LOG_DEBUG("reuse timer [%p]\n",timer);
            }
            vtimer_.erase(vtimer_.begin()+idx);
            removeEvent(timer);
            break;
        }
    }
}

void TimerManager::removeEvent(Timer *timer){
    for(size_t idx = 0;idx<eventVec_.size();idx++){
        if(eventVec_[idx].first == timer){
            event_free(eventVec_[idx].second);
            eventVec_.erase(eventVec_.begin()+idx);
            break;
        }
    }
}

// 单独线程运行
void TimerManager::runInThread(){
    do {
        if (vtimer_.size()) {
            event_base_dispatch(base);
            LOG_DEBUG("timer manager run in thread event after dispatch()\n");
        }
        if(bexit_ == true){
            break;
        }
        std::unique_lock<std::mutex> locker(mutex_);
        expired_cond_.wait(locker);
    } while (!bexit_);
    LOG_DEBUG("time manager run in thread exit\n");
}