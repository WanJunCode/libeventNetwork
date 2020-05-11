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
#include "../System/config.h"

// for logcpp4cpp
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/CategoryStream.hh>

#define LOG_FILE "output/logcpp_file.out"

#define LOG_TRACE(fmt,...) \
    Log::getInstance().printf(Log::TRACE,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
#define LOG_DEBUG(fmt,...) \
    Log::getInstance().printf(Log::DEBUG,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
#define LOG_INFO(fmt,...) \
    Log::getInstance().printf(Log::INFO,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
#define LOG_WARN(fmt,...) \
    Log::getInstance().printf(Log::WARN,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
#define LOG_ERROR(fmt,...) \
    Log::getInstance().printf(Log::ERROR,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
#define LOG_FATAL(fmt,...) \
    Log::getInstance().printf(Log::FATAL,pthread_self(),StripFileName(__FILE__),__LINE__,__FUNCTION__,fmt,##__VA_ARGS__);
    
class Log{
public:
    // 日志等级 枚举类型
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };
    // 单例模式, 静态函数只能访问 静态类成员
    static Log& getInstance(){
        return root_logcpp;
    }
    static Log root_logcpp;

    Log();
    ~Log();

    template<typename T>
    Log& operator << (const T&);

    void printf(LogLevel level,unsigned long pthread_id,const std::string filename,int line,const std::string function,const char *cmd,...);

private:
    mutable std::mutex mutex_;
    std::ofstream out;
    log4cpp::Category& root;
};

template<typename T>
Log& Log::operator << (const T& data){
    out<<data<<std::flush;
    return *this;
}

#endif // !WANJUN_LOG_H