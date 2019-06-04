// check  线程池
#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "Mutex.h"
#include "Thread.h"

#include <deque>    // 队列
#include <vector>
#include <string>

// 线程池不可复制
class MThreadPool : noncopyable{
public:
    typedef std::function<void ()> Task;

    explicit MThreadPool(const std::string& nameArg = std::string("ThreadPool"));
    ~MThreadPool();

    // Must be called before start().
    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task& cb)
    { threadInitCallback_ = cb; }

    void start(int numThreads);
    void stop();

    const std::string& name() const
    { return name_; }

    size_t queueSize() const;

    // 如果最大阻塞队列大于 0 ，可以阻塞
    // Could block if maxQueueSize > 0
    // There is no move-only version of std::function in C++ as of C++14.
    // So we don't need to overload a const& and an && versions
    // as we do in (Bounded)BlockingQueue.
    // https://stackoverflow.com/a/25408989
    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();                    // 获得一个任务

    mutable Mutex mutex_;
    Condition notEmpty_;
    Condition notFull_;
    std::string name_;              // 线程池名称
    Task threadInitCallback_;       // 线程初始化回调函数
    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};

#endif  // MUDUO_BASE_THREADPOOL_H
