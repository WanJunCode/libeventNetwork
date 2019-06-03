#include "Mutex.h"
#include <assert.h>
#include "../Util/utime.h"
#include "../main/logcpp.h"

Monitor::Monitor()
    :ownedMutex_(new Mutex())
    ,mutex_(NULL)
    ,condInitialized_(false){
}

Monitor::Monitor(Mutex* mutex)
    :ownedMutex_(NULL),
    mutex_(NULL),
    condInitialized_(false) {
    init(mutex);
}

Monitor::~Monitor(){
    cleanup();
    delete ownedMutex_;
}

void Monitor::init(Mutex* mutex) {
    mutex_ = mutex;
    if (0 == pthread_cond_init(&pthread_cond_, NULL)) {
        condInitialized_ = true;
    }
    if (!condInitialized_) {
        cleanup();
    }
}

void Monitor::cleanup() {
    if (condInitialized_) {
        condInitialized_ = false;
        pthread_cond_destroy(&pthread_cond_);
    }
}

Mutex& Monitor::mutex() {
    return *mutex_;
}

void Monitor::lock() {
    mutex().lock();
}
void Monitor::unlock() {
    mutex().unlock();
}

void Monitor::notify() {
    pthread_cond_signal(&pthread_cond_);
}

void Monitor::notifyAll() {
    pthread_cond_broadcast(&pthread_cond_);
}

int Monitor::waitForever(){
    pthread_mutex_t* mutexImpl = static_cast<pthread_mutex_t*>(mutex_->getPthreadMutex());
    assert(mutexImpl);
    return pthread_cond_wait(&pthread_cond_, mutexImpl);
}

int Monitor::waitForTime(const timespec* abstime){
    pthread_mutex_t* mutexImpl = static_cast<pthread_mutex_t*>(mutex_->getPthreadMutex());
    assert(mutexImpl);

    // XXX Need to assert that caller owns mutex
    return pthread_cond_timedwait(&pthread_cond_, mutexImpl, abstime);
}

int Monitor::waitForTime(const struct timeval* abstime) {
    struct timespec temp;
    temp.tv_sec = abstime->tv_sec;
    temp.tv_nsec = abstime->tv_usec * 1000;
    return waitForTime(&temp);
}

/**
 * Waits until the specified timeout in milliseconds for the condition to
 * occur, or waits forever if timeout_ms == 0.
 **/
int Monitor::waitForTimeRelative(int64_t timeout_ms){
    if (timeout_ms == 0LL) {
        return waitForever();
    }

    struct timespec abstime;
    Util::toTimespec(abstime, Util::currentTime() + timeout_ms);
    return waitForTime(&abstime);
}

/**
 * Exception-throwing version of waitForTimeRelative(), called simply
 * wait(int64) for historical reasons.  Timeout is in milliseconds.
 *
 * If the condition occurs,  this function returns cleanly; on timeout or
 * error an exception is thrown.
 */
void Monitor::wait(int64_t timeout_ms){
    int result = waitForTimeRelative(timeout_ms);
    if (result != 0) {
        LOG_ERROR("pthread_cond_wait() or pthread_cond_timedwait() failed\n");
    }
}