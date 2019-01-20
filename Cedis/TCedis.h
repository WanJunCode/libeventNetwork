#ifndef __TCEDIS_H
#define __TCEDIS_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <mutex>
#include <hiredis/hiredis.h>
#include "reply.h"

using namespace std;

struct redisContext;
class reply;
class TCedisPool;

class TCedis
{
public:
    typedef std::shared_ptr<TCedis> ptr_t;
    inline static ptr_t create(const std::string& host="localhost",
                               const unsigned int port=6379,
                               const std::string& pass="wanjun"){
        return ptr_t(new TCedis(host, port, pass));
    }

    ~TCedis();

    reply run(const std::vector<std::string>& args);
    reply executeCommand(const char *format, ...);

    bool is_valid() const;

    void Connect(const std::string& host, const unsigned int port,const std::string& pass);
    
    void disconnect(){                                      // 用于测试断开连接
        // std::unique_lock<std::mutex> locker(mutex_);        // 上锁，防止多个线程的争夺   不可省略   会出错 
        std::lock_guard<std::mutex> locker(mutex_);
        redisCommand(c,"QUIT");
    }

    void reConnect(){                                  // 重连
        redisReconnect(c);
    }

    void attach(TCedisPool* pool){
        pool_=pool;
    }

    inline void setUseable(){                      // 设置当前连接为可用状态
        useable = true;                       
    }
    inline bool getuseable(){                      // 获得当前连接的状态
        return useable;
    }

    bool ping();

private:
    TCedis(const std::string& host, const unsigned int port,const std::string& pass);
    void append(const std::vector<std::string>& args);
    reply get_reply();
    std::vector<reply> get_replies(unsigned int count);

public:
    ptr_t self;                             // 保存自己的 指针 ，用于pool 调用 move 存储到queue
private:
    std::mutex mutex_; 
    TCedisPool* pool_;                      // 绑定的连接池
    volatile bool useable;                  // 判断当前连接是否可用
    redisContext *c;                        // hiredis 内部的 connection 
};

#endif