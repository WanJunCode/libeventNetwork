#ifndef __TCEDIS_POOL_H
#define __TCEDIS_POOL_H

#include <string>
#include <memory>
#include <mutex>
#include "TCedis.h"
#include "reply.h"
#include "Queue_s.h"

class TCedis;

class Runnable{
public:
    Runnable(){}
    virtual ~Runnable() {}
    virtual void run() = 0;
};

class TCedisPool:public Runnable {
public:
	TCedisPool( std::string server,
                unsigned int port,
                std::string password,
                unsigned int conns_max,
                unsigned int conns_min);

    void init();
    
    // 防止 赋值 拷贝
	TCedisPool(const TCedisPool&)=delete;
	TCedisPool& operator=(const TCedisPool&)=delete;
    ~TCedisPool();
    
    TCedis::ptr_t grabCedis();                  // 获得一个可用的 TCedis::ptr_t
    void reuseCedis();

    void move(TCedis::ptr_t p){                 // 损坏的连接可以调用 该函数将自己放到 queue中
        queue.push(p);
    }

    // override 表示一定要覆盖基类中的该虚函数
    virtual void run() override{
        while(true){
            std::cout<<"执行 run"<<endl;
            reuseCedis();
        }
    }

private:
    std::mutex  mutex_;                         // 锁

    std::string server_;                        // 服务器ip地址
    unsigned int port_;                         // 端口
    std::string password_;                      // 密码
    unsigned int conns_max_;                    // 连接数量
    unsigned int conns_min_;                    // 连接数量

    std::vector<TCedis::ptr_t> cedisVector;     // 保存所有取出的 TCedis::ptr_t 连接的Vector
    Queue_s<TCedis::ptr_t> queue;               // 不可用的cedis连接
};
#endif