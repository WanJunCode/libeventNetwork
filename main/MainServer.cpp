#include "MainServer.h"
#include "logcpp.h"
#include "IOThread.h"
#include "../Package/ChatPackage.h"
#include "../Package/EchoPackage.h"
#include "../Mysql/MysqlPool.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <chrono>
#include <memory>

#define MAXBUFFERSIZE (1024*16*16)
using namespace std::placeholders;

typedef unsigned char BYTE;

void MainServer::httpcb(const HttpRequest& req,HttpResponse* resp){
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "CloudServer");
    resp->setBody(dispatcher_->debug()+MysqlPool::getInstance()->debug());
}

MainServer::MainServer(size_t port,size_t poolSize,size_t iothreadSize,size_t backlog)
    :port_(port),
    maxBufferSize_(MAXBUFFERSIZE),            // 定义　bufferevent　最大的水位
    threadPoolSize_(poolSize),
    iothreadSize_(iothreadSize),
    backlog_(backlog)
{
    init();
}

MainServer::MainServer(MainConfig *config)
    :config_(config){
    port_ = config_->getPort();
    maxBufferSize_ = config_->getBufferSize();
    threadPoolSize_ = config_->getThreadPoolSize();
    iothreadSize_ = config_->getIOThreadSize();
    backlog_ = config_->getBacklog();

    LOG_DEBUG("port [%lu], maxBufferSize_ = [%lu], threadPoolSize_ = [%lu], iothreadSize_ = [%lu], backlog_ = [%lu]\n",port_,maxBufferSize_,threadPoolSize_,iothreadSize_,backlog_);
    init();
}

MainServer::~MainServer(){
    LOG_DEBUG("main server destructure ...\n");

    for(size_t i=0;i<iothreads_.size();i++){
        iothreads_[i]->breakLoop(false);
    }

    if(redisPool)
        redisPool->exit();
    
    if(timerMgr_)
        timerMgr_->stop();

    // 线程池手动调用结束,防止程序退出时死锁
    threadPool_->stop();
    event_free(ev_stdin);
    event_base_free(main_base);
}

// 主服务器初始化
void MainServer::init(){
    main_base = event_base_new();
    if(main_base == NULL){
        LOG_FATAL("MainServer event base new failure ...\n");
    }

    // 创建服务器监听器
    listener_ = make_shared<PortListener>(backlog_);

    // 创建线程池并设置线程数量
    threadPool_.reset(new MThreadPool("MainThreadPool"));
    threadPool_->setMaxQueueSize(40);//设置线程池的最大线程数量
    threadPool_->start(POOL_SIZE);//开启线程

    // 连接管理器
    dispatcher_.reset(new TConnectionDispatcher(this));

    // TODO  使用信号量同步 IO Thread线程是否全部启动
    // 创建IO Thread vector 并添加到线程池 threadPool_ 中运行
    iothreads_.reserve(iothreadSize_);
    for(size_t i=0;i<iothreadSize_;i++){
        // 创建新的 IOThread 并添加到线程池中
        iothreads_.push_back(std::make_shared<IOThread>(this));
        // std::shared_ptr<>::get() 获得原始指针
        // 将 IOThread::runInThread 函数作为线程执行函数传入
        threadPool_->run(std::bind(&IOThread::runInThread,iothreads_.back().get()));
        // run( std::function<void()> task )
    }

    // TConnection control

    redisPool = make_shared<RedisPool>(config_->redisConfig());
    redisPool->init();
    threadPool_->run(std::bind(&RedisPool::runInThread,redisPool.get()));

    // Mysql Pool
    auto mysqlPool = MysqlPool::getInstance();
    mysqlPool->setParameter("localhost","root","wanjun","cloudserver",3306,NULL,0,20);

    // threadPool_->run(std::bind(&MysqlPool::runInThread,mysqlPool));

    timerMgr_ = make_shared<TimerManager>();
    timerMgr_->init();
    threadPool_->run(std::bind(&TimerManager::runInThread,timerMgr_));

    // 添加需要解析的协议种类
    protocol_ = make_shared<MultipleProtocol>();
    protocol_->addProtocol(std::make_shared<ChatProtocol>());
    protocol_->addProtocol(std::make_shared<EchoProtocol>());
    http.setHttpCallback(std::bind(&MainServer::httpcb,this,_1,_2));
}

// 开启服务器
void MainServer::serve(){
    LOG_DEBUG("serve() ... start \n");
    // 添加一个控制台输入事件
    ev_stdin = event_new(main_base, STDIN_FILENO, EV_READ | EV_PERSIST, stdinCallBack, this);
    event_add(ev_stdin, NULL);

    // 将 MainServer 提供self作为回调函数参数
    // 当有新的连接，会回调 MainServer::handlerConn 函数
    listener_->listen(this,port_);

    // !! import 开启整个程序的主循环
    event_base_dispatch(main_base);
    LOG_DEBUG("serve() ... end \n");
}

// server 处理 来自 transport 的 client_conn
void MainServer::handlerConn(evutil_socket_t client)
{
    dispatcher_->handleConn(client);
}

void MainServer::returnTConnection(TConnection *conn){
    dispatcher_->returnTConnection(conn);
}

bool MainServer::isActive(TConnection *conn) const{
    return dispatcher_->isActive(conn);
}

void MainServer::heartBeat(){
    dispatcher_->heartBeat();
}

void MainServer::execute(std::string cmd,MainServer *server){
    if (cmd == "list"){
#if 0
        LOG_DEBUG("MainServer 激活的 TConnection size %lu\n", server->activeTConnectionVector.size());
        LOG_DEBUG("MainServer 可复用的 TConnection size %lu\n", server->ReuseConnectionQueue.size());
        LOG_DEBUG("PortListener 激活的 TSocket size = %d\n",server->listener_->getActiveSize());
        LOG_DEBUG("PortListener 可复用的 TSocket size = %d\n",server->listener_->getSocketQueue());
#endif
        MysqlPool::getInstance()->debug();
    }else if(cmd == "hb"){
        LOG_DEBUG("heart beat\n");
        server->heartBeat();
    }else{
        // LOG_DEBUG("cmd error...\n");
    }
}

// static
void MainServer::stdinCallBack(evutil_socket_t stdin_fd, short what, void *args){
    UNUSED(what);
    MainServer *server = static_cast<MainServer *>(args);
    char recvline[80];
    bzero(recvline,sizeof(recvline));
    int length = read(stdin_fd, recvline, sizeof(recvline));
    if(length>1)
        length--;
    recvline[length] = '\0';

    std::string cmd(recvline);
    if (cmd == "over"){
        event_base_loopbreak(server->getBase());
    }else{
        execute(cmd,server);
    }  
}

