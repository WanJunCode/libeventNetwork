#include "logcpp.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include "../System/config.h"
#include "../base/CurrentThread.h"

// 日志等级 对应的 字符串，用于将日志等级转化为字符串
const static char *LogLevelName[Log::NUM_LOG_LEVELS] =
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

Log Log::root_logcpp;

Log::Log()
    :root(log4cpp::Category::getRoot())
{
    log4cpp::PatternLayout* pLayout = new log4cpp::PatternLayout();
    pLayout->setConversionPattern("%d: %p %c %x: %m%n");
    log4cpp::Appender* appender = new log4cpp::FileAppender("FileAppender","output/test_logcpp4cpp1.logcpp");
    appender->setLayout(pLayout);
    root.setAppender(appender);
    // Category 需要设置 priority 优先级
    root.setPriority(log4cpp::Priority::DEBUG);
    out.open(LOG_FILE,std::ios::app);
}

Log::~Log(){
    log4cpp::Category::shutdown();
    out.close();
}

void Log::printf(LogLevel level,unsigned long pthread_id,const std::string filename,int line,const std::string function,const char *cmd,...)
{
    UNUSED(pthread_id);
    std::unique_lock<std::mutex> lock(mutex_);

    time_t tt = time(NULL);//这句返回的只是一个时间戳
    struct tm* t= localtime(&tt);
    char timeStr[150]={0};

    sprintf(timeStr,"[%s][%s][%s][%d][%s]:[%02d:%02d:%02d]",LogLevelName[level],CurrentThread::t_threadName,filename.c_str(),line,function.c_str(),t->tm_hour,t->tm_min,t->tm_sec);

    va_list args;       //定义一个va_list类型的变量，用来储存单个参数
    va_start(args,cmd); //使args指向可变参数的第一个参数
    
    // 打印到文件
    Log::getInstance()<<timeStr<<vform(cmd,args);
    // 打印到控制台
    std::cout<<timeStr<<vform(cmd,args);
    // root.notice(vform(cmd,args));
    // 打印到log4cpp
    switch (int(level))
    {
    case LogLevel::TRACE:
        root.notice(vform(cmd,args));
        break;
    case LogLevel::DEBUG:
        root.debug(vform(cmd,args));
        break;
    case LogLevel::INFO:
        root.info(vform(cmd,args));
        break;
    case LogLevel::WARN:
        root.warn(vform(cmd,args));
        break;
    case LogLevel::ERROR:
        root.error(vform(cmd,args));
        break;
    case LogLevel::FATAL:
        root.fatal(vform(cmd,args));
        break;
    default:
        root.notice("default\n");
        break;
    }
    va_end(args);       //结束可变参数的获取
}