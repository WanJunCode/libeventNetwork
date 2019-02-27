#ifndef __MAIN_SERVER_H__
#define __MAIN_SERVER_H__

#include "MyTransport.h"
#include "TSocket.h"
#include "TConnection.h"
#include "ThreadPool.h"
#include "IOThread.h"
#include "Cedis/RedisPool.h"
#include "Package/MultipleProtocol.h"

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
class MainServer{
public:
  MainServer(size_t port = 12345,size_t poolSize = 20,size_t iothreadSize = 10);
  ~MainServer();

  void serve();
  void handlerConn(void *args);
  void returnTConnection(TConnection *conn);
  bool isActive(TConnection *conn);

  std::shared_ptr<ThreadPool> getPool(){
    return thread_pool;
  }
  std::shared_ptr<Protocol> getProtocol(){
    return protocol_;
  }
  struct event_base *getBase(){
    return main_base;
  }
  inline int getBufferSize() const{
    return maxBufferSize_;
  }
  inline Redis *getRedis(){
    return redis_pool->grabCedis();
  }

private:
  struct event *ev_stdin;                     // 处理命令行输入
  struct event_base *main_base;
  
  size_t port_;
  size_t threadPoolSize_;
  size_t iothreadSize_;
  size_t maxBufferSize_;
  size_t selectIOThread_;
  std::mutex connMutex;                       // 处理连接时，以及返回连接时候

  std::shared_ptr<ThreadPool> thread_pool;   // 线程池
  std::shared_ptr<RedisPool> redis_pool;      // redis 连接池
  std::shared_ptr<Protocol> protocol_;        // 协议解析器
  std::shared_ptr<MyTransport> transport_;    // 监听器

  // 使用共享智能指针
  std::vector<std::shared_ptr<IOThread> > iothreads_;

  std::vector<TConnection *> activeTConnection;
  std::queue<TConnection *> connectionQueue;

private:
  static void stdinCallBack(evutil_socket_t stdin_fd, short what, void *args);
};

#endif