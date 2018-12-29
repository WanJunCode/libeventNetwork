#ifndef WANJUN_LOG_H
#define WANJUN_LOG_H

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <mutex>
#include <pthread.h>
#include "Tool.h"           // vform

#define LOG_FILE "log_file.out"

#define LOG_DEBUG(fmt,...) \
    Log::getInstance().printf_w(" tid:[%lu]\t[%s]\t[%d]\t[%s] :\t",pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__);\
    Log::getInstance().printf_w_notime(fmt, ##__VA_ARGS__);

class Log{
public:
    Log();
    ~Log();

    template<typename T>
    Log& operator << (const T&);
    void printf_w(const char *cmd, ...);
    void printf_w_notime(const char *cmd, ...);


    // 单例模式
    static Log& getInstance();
    static Log root_log;

private:
    std::ofstream out;
    std::mutex mutex_;
};

template<typename T>
Log& Log::operator << (const T& data){
    // std::unique_lock<std::mutex> locker(mutex_);
    out<<data<<std::flush;
}

#endif // !WANJUN_LOG_H