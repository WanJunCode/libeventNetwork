#ifndef __MAIN_SERVER_H__
#define __MAIN_SERVER_H__

#include "MyTransport.h"
#include "TSocket.h"
#include "TConnection.h"
#include "ThreadPool.h"
#include "IOThread.h"
#include "Cedis/RedisPool.h"
#include "Package/MultipleProtocol.h"

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
class MainServer
{
public:
  explicit MainServer();
  ~MainServer();

  struct event_base *getBase();

  void serve();

  void handlerConn(void *args);
  void returnTConnection(TConnection *conn);

  ThreadPool *getPool();

  bool isActive(TConnection *conn);
  inline int getBufferSize() const{
    return maxBufferSize;
  }
  inline Redis *getRedis(){
    return redis_pool->grabCedis();
  }

  Protocol *getProtocol(){
    return protocol;
  }

private:
  struct event_base *main_base;
  struct event *ev_stdin;     // 处理命令行输入
  
  int port_;
  int maxBufferSize;
  ThreadPool *thread_pools;

  // 使用共享智能指针
  std::vector<std::shared_ptr<IOThread> > iothreads_;

  MyTransport *transport_;    // 监听 tcp 连接
  std::mutex connMutex;

  std::vector<TConnection *> activeTConnection;
  std::queue<TConnection *> connectionQueue;
  std::shared_ptr<RedisPool> redis_pool;

  int selectIOThread;
  Protocol *protocol;

private:
  static void stdin_cb(evutil_socket_t stdin_fd, short what, void *args);
};

#endif