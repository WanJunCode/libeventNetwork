#ifndef WJ_MUTEX
#define WJ_MUTEX

#include <pthread.h>    // mutex condition_variable
#include <stdint.h>
#include "noncopyable.h"

// class Mutex
// class MutexGuard
// class Condition
// class Monitor

class Mutex: noncopyable{
private:
    pthread_mutex_t mutex;
public:
    Mutex(){
        pthread_mutex_init(&mutex,NULL);
    }
    ~Mutex(){
        pthread_mutex_destroy(&mutex);
    }
    void lock(){
        pthread_mutex_lock(&mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&mutex);
    }
    bool trylock(){
        return 0 == pthread_mutex_trylock(&mutex);
    }
    pthread_mutex_t* getPthreadMutex(){
        return &mutex;
    }
};

class MutexGuard: noncopyable{
private:
    Mutex& mutex_;
public:
    explicit MutexGuard(Mutex& mutex)
    :mutex_(mutex){
        mutex_.lock();
    }
    ~MutexGuard(){
        mutex_.unlock();
    }
};

// Condition 类的构造函数必须要 Mutex
class Condition: noncopyable{
private:
    Mutex&          mutex_;
    pthread_cond_t  pcond_;
public:
    explicit Condition(Mutex& mutex)
    :mutex_(mutex){
        pthread_cond_init(&pcond_,NULL);
    }
    ~Condition(){
        pthread_cond_destroy(&pcond_);
    }
    void wait(){
        pthread_cond_wait(&pcond_,mutex_.getPthreadMutex());
    }
    void notify(){
        pthread_cond_signal(&pcond_);
    }
    void notifyAll(){
        pthread_cond_broadcast(&pcond_);
    }
};

// monitor有两种mutex,可以是自己创建的,也可以是从外部获得的
class Monitor: noncopyable {
public:
    /** Creates a new mutex, and takes ownership of it. */
    Monitor();
    /** Uses the provided mutex without taking ownership. 极少情况下使用*/
    explicit Monitor(Mutex* mutex);
    /** Deallocates the mutex only if we own it. */
    ~Monitor();

public:
    Mutex& mutex();
    void lock();
    void unlock();

    /** Wakes up one thread waiting on this monitor. */
    void notify();
    /** Wakes up all waiting threads on this monitor. */
    void notifyAll();

    /**
     * Waits forever until the condition occurs.
     * Returns 0 if condition occurs, or an error code otherwise.
     */
    int waitForever();

    /**
     * Waits until the absolute time specified using struct timeval.
     * Returns 0 if condition occurs, THRIFT_ETIMEDOUT on timeout, or an error code.
     */
    int waitForTime(const timespec* abstime);

    /**
     * Waits until the absolute time specified using struct THRIFT_TIMESPEC.
     * Returns 0 if condition occurs, THRIFT_ETIMEDOUT on timeout, or an error code.
     */
    int waitForTime(const struct timeval* abstime);

    /**
     * Waits a maximum of the specified timeout in milliseconds for the condition
     * to occur, or waits forever if timeout_ms == 0.
     * Returns 0 if condition occurs, THRIFT_ETIMEDOUT on timeout, or an error code.
     */
    int waitForTimeRelative(int64_t timeout_ms);

    /**
     * Exception-throwing version of waitForTimeRelative(), called simply
     * wait(int64) for historical reasons.  Timeout is in milliseconds.
     * If the condition occurs,  this function returns cleanly; on timeout or
     * error an exception is thrown.
     */
    void wait(int64_t timeout_ms = 0LL);
private:
    void init(Mutex* mutex);
    void cleanup();
private:
    Mutex* ownedMutex_;
    Mutex* mutex_;
    pthread_cond_t pthread_cond_;
    mutable bool condInitialized_;
};

#endif // !WJ_MUTEX
