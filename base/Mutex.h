#ifndef WJ_MUTEX
#define WJ_MUTEX

#include <pthread.h>    // mutex condition_variable
#include <stdint.h>
#include <assert.h>
#include "CurrentThread.h"
#include "noncopyable.h"

// class Mutex
// class MutexGuard
// class Condition
// class Monitor

class Mutex: noncopyable{
private:
    pthread_mutex_t mutex;
    pid_t holder_;
public:
    Mutex()
    :holder_(0){
        pthread_mutex_init(&mutex,NULL);
    }
    ~Mutex(){
        assert(holder_ == 0);
        pthread_mutex_destroy(&mutex);
    }
    void lock(){
        pthread_mutex_lock(&mutex);
        assignHolder();
    }
    void unlock(){
        unassignHolder();
        pthread_mutex_unlock(&mutex);
    }
    // must be called when locked, i.e. for assertion
    bool isLockedByThisThread() const{
        return holder_ == CurrentThread::tid();
    }
    void assertLocked() const{
        assert(isLockedByThisThread());
    }
    pthread_mutex_t* getPthreadMutex(){
        return &mutex;
    }
private:
    friend class Condition;
    // 类中类，守护未被分配； 创建 UnassignGuard 传入 MutexLock 构造后，将 holder清除
    class UnassignGuard : noncopyable{
    public:
        explicit UnassignGuard(Mutex& owner)
        : owner_(owner){
            owner_.unassignHolder();
        }
        ~UnassignGuard(){
            owner_.assignHolder();
        }
    private:
        Mutex& owner_;
    };
    void assignHolder(){
        // 赋值，使用线程id
        holder_ = CurrentThread::tid();
    }
    void unassignHolder(){
        holder_ = 0;
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
    Mutex&          mutex_; // 引用
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
        // 条件变量等待的时候 mutex 不会被上锁
        Mutex::UnassignGuard ug(mutex_);
        pthread_cond_wait(&pcond_,mutex_.getPthreadMutex());
    }
    bool waitForSeconds(double seconds){
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        const int64_t kNanoSecondsPerSecond = 1000000000;
        int64_t nanoseconds = static_cast<time_t>(seconds * kNanoSecondsPerSecond);
        abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
        abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);
        Mutex::UnassignGuard ug(mutex_);
        return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime); 
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
