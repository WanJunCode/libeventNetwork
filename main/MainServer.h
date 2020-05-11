#ifndef LIBNETWORK_MAIN_MAINSERVER_H_
#define LIBNETWORK_MAIN_MAINSERVER_H_

#include "PortListener.h"
#include "TSocket.h"
#include "ThreadPool.h"
#include "IOThread.h"
#include "MainConfig.h"
#include "TConnectionDispatcher.h"
#include "TConnectionDispatcher.h"

#include "../Cedis/RedisPool.h"
#include "../Package/MultipleProtocol.h"
#include "../Package/Adapter/Adapter.h"
#include "../base/ThreadPool.h"
#include "../base/TimerManager.h"
#include "../base/noncopyable.h"
#include "../http/httpServer.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"

#include <event.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

#define PORT 12345
#define POOL_SIZE 20
#define IOTHREAD_SIZE 10

//　尽量避免使用前置声明
class TConnectionDispatcher;

class MainServer: public noncopyable{
public:
    MainServer(size_t port = 12345, size_t poolSize = 20, size_t iothreadSize = 10, size_t backlog = 10);
    MainServer(MainConfig *config);
    void init();
    ~MainServer();

    void serve();
    void handlerConn(evutil_socket_t client);
    void returnTConnection(TConnection *conn);
    bool isActive(TConnection *conn) const;
    void heartBeat();

    void run(MThreadPool::Task task){
        threadPool_->run(task);
    }

    std::shared_ptr<Protocol> getProtocol(){
        return protocol_;
    }
    struct event_base *getBase(){
        return main_base;
    }
    inline std::shared_ptr<Redis> getRedis(){
        return redisPool->grabCedis();
    }
    inline int getBufferSize() const{
        return maxBufferSize_;
    }
    HttpServer& gethttp(){
        return http;
    }
    std::shared_ptr<PortListener> getListener(){
        return listener_;
    }

    IOThread *getRandomIOThread(){
        static size_t selectNum = 0;
        selectNum++;
        // load balance
        selectNum = selectNum %  iothreads_.size();
        return iothreads_[selectNum].get();
    }

    void httpcb(const HttpRequest& req,HttpResponse* resp);

private:
    struct event *ev_stdin; // 处理命令行输入
    struct event_base *main_base;

    MainConfig *config_;

    size_t port_;
    size_t maxBufferSize_;
    size_t threadPoolSize_;
    size_t iothreadSize_;
    size_t backlog_;

    std::unique_ptr<MThreadPool> threadPool_;
    std::shared_ptr<TConnectionDispatcher> dispatcher_;
    std::shared_ptr<RedisPool> redisPool;   	// redis 连接池
    std::shared_ptr<Protocol> protocol_;     	// 协议解析器
    std::shared_ptr<PortListener> listener_; 	// 监听器
    std::shared_ptr<TimerManager> timerMgr_;
    // 使用共享智能指针
    std::vector<std::shared_ptr<IOThread>> iothreads_;

    HttpServer http;

private:
    static void stdinCallBack(evutil_socket_t stdin_fd, short what, void *args);
    static void execute(std::string cmd, MainServer *server);
};

#endif // LIBNETWORK_MAIN_MAINSERVER_H_