#ifndef __Redis_POOL_H
#define __Redis_POOL_H

#include <string>
#include <memory>
#include <mutex>
#include "Redis.h"
#include "Reply.h"
#include "Queue_s.h"

class Runnable{
public:
    Runnable(){}
    virtual ~Runnable() {}
    virtual void run() = 0;
};

class RedisPool:public Runnable {
public:
	RedisPool(  std::string server,
                unsigned int port,
                std::string password,
                unsigned int conns_max,
                unsigned int conns_min);
    
    // 防止 赋值 拷贝, 数据库连接库不能复制和赋值
	RedisPool(const RedisPool&)=delete;
	RedisPool& operator=(const RedisPool&)=delete;
    ~RedisPool();
    void init();
    
    Redis *grabCedis();                      // 获得一个可用的 Redis::ptr_t
    void reuseCedis();

    inline void move(Redis *p){                  // 损坏的连接可以调用 该函数将自己放到 queue中
        queue.push(p);
    }

    // override 表示一定要覆盖基类中的该虚函数
    virtual void run() override{
        while(!stop){
            reuseCedis();
        }
    }

    void operator()(){
        while(!stop){
            reuseCedis();
        }
    }

    void exit(){
        stop = true;
        queue.wake_all();
    }

private:
    volatile bool           stop;
    std::mutex              mutex_;             // 锁
    std::string             server_;            // 服务器ip地址
    unsigned int            port_;              // 端口
    std::string             password_;          // 密码
    unsigned int            conns_max_;         // 连接数量
    unsigned int            conns_min_;         // 连接数量

    std::vector<Redis *>    redis_vec;          // 保存所有取出的 Redis::ptr_t 连接的Vector
    Queue_s<Redis *>        queue;              // 不可用的cedis连接
};
#endif