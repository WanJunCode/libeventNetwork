// redis pool 设计的目的是复用 redis 连接
// redispool 使用 vector 容器存储 redis 连接
// grabCedis 返回一个可用的连接
// redis 使用过程中如果断开则将该redis的状态设置为不可用
// 如果redisPool在redis的生命周期前析构，使用弱引用将redis本身添加到等待队列 queue 中重连

#ifndef __Redis_POOL_H
#define __Redis_POOL_H

#include <string>
#include <memory>
#include <mutex>
#include "Redis.h"
#include "Reply.h"
#include "Queue_s.h"
#include "../base/noncopyable.h"

class RedisConfig;
class RedisPool: public noncopyable, public std::enable_shared_from_this<RedisPool>{
public:
	RedisPool(  std::string server,
                unsigned int port,
                std::string password,
                unsigned int conns_max,
                unsigned int conns_min);
    RedisPool(std::shared_ptr<RedisConfig> config);             // 配置文件是否需要使用共享智能指针
    ~RedisPool();
    void init();
    
    std::shared_ptr<Redis> grabCedis();                         // 获得一个可用的 Redis::ptr_t
    void reuseCedis();

    void move(std::shared_ptr<Redis> p){                        // 损坏的连接可以调用 该函数将自己放到 queue中
        queue.push(p);
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
    mutable std::mutex      mutex_;             // 锁
    std::string             server_;            // 服务器ip地址
    unsigned int            port_;              // 端口
    std::string             password_;          // 密码
    unsigned int            conns_max_;         // 连接数量
    unsigned int            conns_min_;         // 连接数量
    volatile bool           stop;               // reids pool 状态

    std::vector<std::shared_ptr<Redis> >    redisVec;          // 保存所有取出的 Redis::ptr_t 连接的Vector
    Queue_s<std::shared_ptr<Redis> >        queue;             // 不可用的cedis连接
};
#endif