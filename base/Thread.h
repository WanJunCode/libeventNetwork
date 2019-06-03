#ifndef WJ_BASE_THREAD_H
#define WJ_BASE_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "noncopyable.h"

#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;

  explicit Thread(ThreadFunc, const std::string& name = std::string());
  ~Thread();

  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return name_; }

  // 类 静态函数
  static int numCreated() { return numCreated_.get(); }

 private:
  void setDefaultName();

  bool       started_;      // 线程开始状态
  bool       joined_;       // 线程是否 joined
  pthread_t  pthreadId_;    
  pid_t      tid_;          // process identifications
  ThreadFunc func_;         // 线程执行函数
  std::string     name_;         // 线程名称
  CountDownLatch latch_;
  // 静态数据类型，用于记录创建了多少个线程
  static AtomicInt32 numCreated_;
};

#endif  // WJ_BASE_THREAD_H