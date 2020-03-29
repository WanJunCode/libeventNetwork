// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "Thread.h"
#include "CurrentThread.h"
#include "../main/logcpp.h"

#include <type_traits>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

// 防止程序调用fork后看到缓存结果
void afterFork(){
    // 给 fork 后的进程设置主线程名称
    CurrentThread::t_cachedTid = 0;
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

// 线程名称 初始器
class ThreadNameInitializer
{
    public:
    ThreadNameInitializer()
    {
        CurrentThread::t_threadName = "main";
        // 缓存线程id
        CurrentThread::tid();
        // 设置线程 fork 后需要调用的函数
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

// 定义一个全局变量 线程名称 初始器
ThreadNameInitializer init;

// 线程数据
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;         // 线程执行函数
    std::string name_;        // 线程名称
    pid_t* tid_;              // 线程 id
    CountDownLatch* latch_;   // 用于指明线程是否 执行完成

    ThreadData(ThreadFunc func,
                const std::string& name,
                pid_t* tid,
                CountDownLatch* latch)
        : func_(std::move(func)),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }

    // 在线程中执行的函数
    void runInThread()
    {
        // 获得当前线程id
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        // 赋值线程名称
        CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
        ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
        try
        {
            // 执行该函数
            func_();
            // 设置当前线程名称为 finished
            CurrentThread::t_threadName = "finished";
        }
        catch (const std::exception& ex)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    }
};//struct ThreadData

// 开始执行的线程函数
void* startThread(void* obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

// 定义 原子变量 用于记录创建了多少线程
AtomicInt32 Thread::numCreated_;

Thread::Thread(ThreadFunc func, const std::string& n)
// 3.Thread(ThreadFunc func, const string& n)
  : started_(false),
    joined_(false),     // 默认是不需要等待线程结束
    pthreadId_(0),
    tid_(0),
    func_(std::move(func)),
    name_(n),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        pthread_detach(pthreadId_);
    }
}

// 使用 原子变量 设置 Thread id
void Thread::setDefaultName()
{
    int num = numCreated_.incrementAndGet();
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    // 创建一个 ThreadData 将线程数据 obj 传入线程函数中
    ThreadData* threadData = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, threadData))
    {
        // 线程创建失败
        started_ = false;
        delete threadData; // or no delete?
        LOG_ERROR("Failed in pthread_create\n");
    }
    else
    {
        // 在此等待 线程函数初始化完毕
        latch_.wait();
        LOG_DEBUG("thread [%d] initial successful\n",name());
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    // 等待函数执行完毕
    return pthread_join(pthreadId_, NULL);
}