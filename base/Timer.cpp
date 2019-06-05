#include "Timer.h"
#include "TimerManager.h"
#include "../main/logcpp.h"

Timer::Timer(std::shared_ptr<TimerManager> tmMgr)
    : task_(nullptr)
    , owned_(true) {
    tmMgr_ = tmMgr;
    LOG_DEBUG("timer init\n");
}

Timer::~Timer() {
    LOG_DEBUG("timer destory\n");
}

void Timer::start(std::function<void()> task, unsigned interval, eTimerType etype) {
    task_ = task;
    interval_ = interval;
    type_ = etype;
    auto mgr = tmMgr_.lock();
    if(mgr){
        mgr->addTimer(this);
    }else{
        LOG_INFO("timer manager is gone\n");
    }
}

void Timer::returnTimer() {
    auto mgr = tmMgr_.lock();
    if(mgr){
        mgr->returnTimer(this);
    }else{
        LOG_INFO("timer manager is gone\n");
    }
}

void Timer::run() {
    if (task_) {
        task_();
    }
};

void Timer::reset() {
    interval_ = 0;
    task_ = nullptr;
};
