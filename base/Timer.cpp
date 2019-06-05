#include "Timer.h"
#include "TimerManager.h"
#include "../main/logcpp.h"

Timer::Timer(TimerManager *tmMgr)
    : tmMgr_(tmMgr)
    , task_(nullptr)
    , owned_(true) {
    LOG_DEBUG("timer init\n");
}

Timer::~Timer() {
    LOG_DEBUG("timer destory\n");
}

void Timer::start(std::function<void()> task, unsigned interval, eTimerType etype) {
    task_ = task;
    interval_ = interval;
    type_ = etype;
    tmMgr_->addTimer(this);
}

void Timer::returnTimer() {
    tmMgr_->returnTimer(this);
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
