#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <stdint.h>
#include <string>

namespace CurrentThread
{
  // __thread是GCC内置的线程局部存储设施，存取效率可以和全局变量相比。__thread变量每一个线程有一份独立实体，
  // 各个线程的值互不干扰。可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。

  // 声明，定义在 .cpp 文件
  extern __thread int t_cachedTid;
  // 存储线程id字符串
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName; // 用于记录当前线程的状态

  void cacheTid();

  inline int tid(){
    // 更倾向于 t_cachedTid == 0 是 fasle，即 t_cachedTid ！= 0
    if (__builtin_expect(t_cachedTid == 0, 0)){
      cacheTid();
    }
    return t_cachedTid;
  }

  // 返回线程id的字符串形式
  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  // 线程id的字符串长度
  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }

  // 获得线程名称
  inline const char* name()
  {
    return t_threadName;
  }

  // 判断是否是主线程
  bool isMainThread();

  void sleepUsec(int64_t usec);  // for testing

}  // namespace CurrentThread

#endif  // CURRENTTHREAD_H