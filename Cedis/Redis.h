#ifndef __Redis_H
#define __Redis_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <mutex>
#include <hiredis/hiredis.h>
#include "Reply.h"

using namespace std;

struct redisContext;
class Reply;
class RedisPool;

class Redis{
public:
    typedef std::shared_ptr<Redis> ptr_t;
    inline static ptr_t create(const std::string& host="localhost",
                               const unsigned int port=6379,
                               const std::string& pass="wanjun"){
        return ptr_t(new Redis(host, port, pass));
    }
    Redis(const std::string& host, const unsigned int port,const std::string& pass);
    ~Redis();

    Reply run(const std::vector<std::string>& args);
    Reply executeCommand(const char *format, ...);

    bool is_valid() const;

    void Connect(const std::string& host, const unsigned int port,const std::string& pass);
    
    void disconnect(){                                      // 用于测试断开连接
        // 上锁，防止多个线程的争夺   不可省略   会出错 
        std::lock_guard<std::mutex> locker(mutex_);
        redisCommand(conn_,"QUIT");
    }

    void reConnect(){                                  // 重连
        redisReconnect(conn_);
    }

    void attach(RedisPool* pool){
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
    void append(const std::vector<std::string>& args);
    Reply get_reply();
    std::vector<Reply> get_replies(unsigned int count);

public:
    static int count;

private:
    std::mutex mutex_; 
    RedisPool* pool_;                      // 绑定的连接池
    volatile bool useable;                  // 判断当前连接是否可用
    redisContext *conn_;                        // hiredis 内部的 connection 
};


#endif