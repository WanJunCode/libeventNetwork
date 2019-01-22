#include "log.h"

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

Log Log::root_log;

Log::Log():
    root(log4cpp::Category::getRoot()){

    log4cpp::PatternLayout* pLayout = new log4cpp::PatternLayout();
    pLayout->setConversionPattern("%d: %p %c %x: %m%n");
    log4cpp::Appender* appender = new log4cpp::FileAppender("FileAppender","./test_log4cpp1.log");
    appender->setLayout(pLayout);
    root.setAppender(appender);
    // Category 需要设置 priority 优先级
    root.setPriority(log4cpp::Priority::INFO);

    out.open(LOG_FILE,std::ios::app);
}

Log::~Log(){
    log4cpp::Category::shutdown();
    out.close();
}

Log& Log::getInstance(){
    return root_log;
}

void Log::printf_w(const char *cmd, ...){
    // unique_lock ＆　lock_guard 区别
	// std::unique_lock<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> lock(mutex_);

    time_t tt = time(NULL);//这句返回的只是一个时间cuo
    struct tm* t= localtime(&tt);
    char timeStr[50]={0};
    sprintf(timeStr,"[%02d:%02d:%02d]",t->tm_hour,t->tm_min,t->tm_sec);
    va_list args;       //定义一个va_list类型的变量，用来储存单个参数
    va_start(args,cmd); //使args指向可变参数的第一个参数
    // Log::getInstance()<<timeStr<<vform(cmd,args);
    std::string str = vform(cmd,args);
    Log::getInstance()<<timeStr<<str;
    root.info(str);
    va_end(args);       //结束可变参数的获取
}

void Log::printf(unsigned long pthread_id,const std::string filename,int line,const std::string function,const char *cmd,...)
{
    std::unique_lock<std::mutex> lock(mutex_);

    time_t tt = time(NULL);//这句返回的只是一个时间戳
    struct tm* t= localtime(&tt);
    char timeStr[150]={0};
    // string printf
    sprintf(timeStr,"tid:[%lu]\t[%s]\t[%d]\t[%s] :\t[%02d:%02d:%02d]",pthread_id,filename.c_str(),line,function.c_str(),t->tm_hour,t->tm_min,t->tm_sec);
    va_list args;       //定义一个va_list类型的变量，用来储存单个参数
    va_start(args,cmd); //使args指向可变参数的第一个参数
    
    // Log::getInstance()<<timeStr<<vform(cmd,args);
    Log::getInstance()<<timeStr<<vform(cmd,args);
    std::cout<<timeStr<<vform(cmd,args);
    root.info(vform(cmd,args));
    va_end(args);       //结束可变参数的获取
}

void Log::printf_w_notime(const char *cmd, ...){
	std::unique_lock<std::mutex> lock(mutex_);
    va_list args;       //定义一个va_list类型的变量，用来储存单个参数
    va_start(args,cmd); //使args指向可变参数的第一个参数
    Log::getInstance()<<vform(cmd,args);
    va_end(args);       //结束可变参数的获取
}