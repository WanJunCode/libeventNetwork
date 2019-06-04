#ifndef __MAIN_SERVER_H__
#define __MAIN_SERVER_H__

#include "MyTransport.h"
#include "TSocket.h"
#include "TConnection.h"
#include "ThreadPool.h"
#include "IOThread.h"
#include "MainConfig.h"
#include "../Cedis/RedisPool.h"
#include "../Package/MultipleProtocol.h"
#include "../base/ThreadPool.h"

// #include "grpc/client.h"

#include <event.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

#define PORT 12345
#define POOL_SIZE 20
#define IOTHREAD_SIZE 10

class TSocket;
class MyTransport;
class TConnection;
class Protocol;
class MainConfig;

class MainServer{
public:
    MainServer(size_t port = 12345, size_t poolSize = 20, size_t iothreadSize = 10, size_t backlog = 10);
    MainServer(MainConfig *config);
    void init();
    ~MainServer();

    void serve();
    void handlerConn(void *args);
    void returnTConnection(TConnection *conn);
    bool isActive(TConnection *conn) const;
    void heartBeat();

    std::shared_ptr<ThreadPool> getPool(){
      return thread_pool;
    }
    std::shared_ptr<Protocol> getProtocol(){
      return protocol_;
    }
    struct event_base *getBase(){
      return main_base;
    }
    inline std::shared_ptr<Redis> getRedis(){
      return redis_pool->grabCedis();
    }
    inline int getBufferSize() const{
      return maxBufferSize_;
    }

private:
    struct event *ev_stdin; // 处理命令行输入
    struct event_base *main_base;

    MainConfig *config_;

    size_t port_;
    size_t selectIOThread_;
    size_t maxBufferSize_;
    size_t threadPoolSize_;
    size_t iothreadSize_;
    size_t backlog_;
    std::mutex connMutex; // 处理连接时，以及返回连接时候

    std::shared_ptr<ThreadPool> thread_pool; // 线程池
    std::shared_ptr<RedisPool> redis_pool;   // redis 连接池
    std::shared_ptr<Protocol> protocol_;     // 协议解析器
    std::shared_ptr<MyTransport> transport_; // 监听器

    // 使用共享智能指针
    std::vector<std::shared_ptr<IOThread>> iothreads_;
    // 活动的连接
    std::vector<TConnection *> activeTConnection;
    std::queue<TConnection *> connectionQueue;
    std::unique_ptr<MThreadPool> threadPool_;

private:
    static void stdinCallBack(evutil_socket_t stdin_fd, short what, void *args);
    static void execute(std::string cmd, MainServer *server);
};

#endif