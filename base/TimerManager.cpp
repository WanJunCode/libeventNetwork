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
    event_free(ev_breakloop);
    event_base_free(base);

    close(breakloop[0]);
    close(breakloop[1]);

    LOG_DEBUG("timer manager des\n");
}

void TimerManager::init(){
    if(0!=pipe(breakloop)){
        LOG_ERROR("timer manager pipe don't work\n");
    }

    ev_breakloop = event_new(base, breakloop[0], EV_READ | EV_PERSIST, eventBreakLoop, base);
    event_add(ev_breakloop, NULL);

    // 共享指针存入栈中
    for (int i = 0; i < 10; ++i) {
        timerStack_.push(new Timer(shared_from_this()));
    }
    LOG_DEBUG("timer manager init\n");
}

// stop 在主线程调用
void TimerManager::stop(){
    bexit_ = true;


    std::lock_guard<std::mutex> locker(mutex_);
    while (vtimer_.size()) {
        Timer *ref = vtimer_.back();
        // 结束在 event 上注册的定时器
        removeEvent(ref);
        vtimer_.pop_back();
        delete ref;
    }

    while(timerStack_.size() > 0){
        Timer *ref = timerStack_.top();
        timerStack_.pop();
        delete ref;
    }
    
    const char msg[] = "exit";
    write(breakloop[1],msg,sizeof(msg));

    LOG_DEBUG("timer manager stop event loop break\n");
    expired_cond_.notify_one();
}

Timer *TimerManager::grabTimer(){
    Timer *timer = nullptr;
    if (timerStack_.empty()) {
        timer = new Timer(shared_from_this());
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
        if(vtimer_[idx]==timer){
            if(timerStack_.size()<TIMER_STACK_SIZE){
                timerStack_.push(vtimer_[idx]);
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
            event_del(eventVec_[idx].second);
            event_free(eventVec_[idx].second);
            LOG_DEBUG("释放 event\n");
            eventVec_.erase(eventVec_.begin()+idx);
            break;
        }
    }
}

// static 
void TimerManager::eventBreakLoop(int fd, short event, void *arg){
    struct event_base *base=(struct event_base *)arg;
    char buffer[1024];    
    int length = read(fd,buffer,sizeof buffer);
    LOG_DEBUG("回调函数 read length [%d]\n",length);
    event_base_loopbreak(base);
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