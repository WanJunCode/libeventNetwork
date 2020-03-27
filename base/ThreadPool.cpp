#include "ThreadPool.h"
#include "../main/logcpp.h"
#include <assert.h>
#include <stdio.h>

MThreadPool::MThreadPool(const std::string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false){
    LOG_DEBUG("muduo thread pool init\n");
}

MThreadPool::~MThreadPool(){
    // 如果正在运行则 stop
    if (running_){
        LOG_INFO("muduo thread pool stop\n");
        stop();
    }
    LOG_INFO("muduo thread pool desc\n");
}

// 开启线程池，参数numThreads表示线程数量
void MThreadPool::start(int numThreads){
    assert(threadVector.empty());
    running_ = true;
    threadVector.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i){
        char id[32];
        snprintf(id, sizeof id, "%d", i+1);
        // 绑定 pool 的 runInThread 函数
        // Thread都会执行MThreadPool::runInThread函数
        threadVector.emplace_back(new Thread(std::bind(&MThreadPool::runInThread, this), 
                                name_+id));
        threadVector[i]->start();
    }
    // 如果不是多线程 并且有线程初始化回调函数
    if (numThreads == 0 && threadInitCallback_){
      threadInitCallback_();
    }
}

void MThreadPool::stop(){
    {
        MutexGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();      // 广播让所有的子线程结束
    }
    for (auto& thr : threadVector){
        thr->join();
    }
}

size_t MThreadPool::queueSize() const{
    MutexGuard lock(mutex_);
    return queue_.size();
}

// 存入新的任务
void MThreadPool::run(Task task){
    // LOG_DEBUG("thread pool run 新的任务\n");
    // 如果是单线程(子线程容器为空)，直接运行 task
    if (threadVector.empty()){
        task();
    }else{
        MutexGuard lock(mutex_);
        // 在工作线程都满的情况 等待
        while (isFull()){
            notFull_.wait();  // 等待未满的条件
        }
        assert(!isFull());
        // 将 task 添加到任务队列中，提醒工作线程去执行
        queue_.push_back(std::move(task));
        notEmpty_.notify(); // 提示未空，有新的任务添加了
    }
}

MThreadPool::Task MThreadPool::take(){
    MutexGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    // 等待工作队列中的元素
    while (queue_.empty() && running_){
        notEmpty_.wait();
    }
    // std::funtional<void()>
    Task task;
    if (!queue_.empty()){
        task = queue_.front();
        queue_.pop_front();
        // 取出一个任务后 ， 通知线程池可以放入新的任务
        if (maxQueueSize_ > 0){
            notFull_.notify();
        }
    }
    return task;
}

// 判断 任务队列是否已满
bool MThreadPool::isFull() const{
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void MThreadPool::runInThread(){
    try
    {
        // 执行 线程初始化回调函数
        if (threadInitCallback_){
            threadInitCallback_();
        }while (running_){
            // std::funtional<void()>
            Task task(take());
            if (task){
                task();
            }
        }
    }catch (const std::exception& ex){
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }catch (...){
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

