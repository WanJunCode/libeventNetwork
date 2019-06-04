#include "CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include "../Util/utime.h"

namespace CurrentThread
{
  // 定义
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "unknown";


// 获得线程id
pid_t gettid(){
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

// 缓存 线程id
void cacheTid(){
    if (t_cachedTid == 0){
      t_cachedTid = gettid();
      // string number printf
      t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

// 判断是否主线程id,主线程id和进程id一致
bool isMainThread(){
    // 当前线程缓存的id 和 进程id 的比较
    return tid() == ::getpid();
}

// 1000000 表示 1 秒
void sleepUsec(int64_t usec){
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Util::US_PER_S);
    ts.tv_nsec = static_cast<long>(usec % Util::NS_PER_S);
    ::nanosleep(&ts, NULL);
}

}  // namespace CurrentThread