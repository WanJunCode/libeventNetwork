#ifndef WJ_BASE_THREAD_H
#define WJ_BASE_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "noncopyable.h"

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

// 使用 std::function 降低各个模块之间的耦合性
class Thread: noncopyable{
public:
    typedef std::function<void ()> ThreadFunc;  // 可执行函数,表示任务

    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    // 开启线程
    void start();
    // 等待线程结束
    int join();
    // 判断线程是否已经开启
    bool started() const { return started_; }
    // pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return tid_; }
    // 返回线程名称
    const std::string& name() const { return name_; }
    // 类静态函数:获得当前已经创建的线程数量
    static int numCreated() { return numCreated_.get(); }

private:
    void setDefaultName();

    bool       started_;      // 线程开始状态
    bool       joined_;       // 线程是否 joined
    pthread_t  pthreadId_;    // 线程ID
    pid_t      tid_;          // process identifications
    ThreadFunc func_;         // 线程执行函数
    std::string     name_;    // 线程名称
    CountDownLatch latch_;    // 倒计时
    // 静态数据类型，用于记录创建了多少个线程
    static AtomicInt32 numCreated_;
};

#endif  // WJ_BASE_THREAD_H