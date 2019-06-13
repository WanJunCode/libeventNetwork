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

typedef unsigned char BYTE;

MainServer::MainServer(size_t port,size_t poolSize,size_t iothreadSize,size_t backlog)
    :port_(port),
    selectIOThread_(0),
    maxBufferSize_(MAXBUFFERSIZE),            // 定义　bufferevent　最大的水位
    threadPoolSize_(poolSize),
    iothreadSize_(iothreadSize),
    backlog_(backlog)
{
    init();
}

MainServer::MainServer(MainConfig *config)
    :config_(config){
    selectIOThread_ = 0;
    port_ = config_->getPort();
    maxBufferSize_ = config_->getBufferSize();
    threadPoolSize_ = config_->getThreadPoolSize();
    iothreadSize_ = config_->getIOThreadSize();
    backlog_ = config_->getBacklog();

    LOG_DEBUG("port [%lu], maxBufferSize_ = [%lu], threadPoolSize_ = [%lu], iothreadSize_ = [%lu], backlog_ = [%lu]\n",port_,maxBufferSize_,threadPoolSize_,iothreadSize_,backlog_);
    init();
}

void MainServer::init(){
    main_base = event_base_new();
    if(main_base == NULL){
        LOG_FATAL("MainServer event base new failure ...\n");
    }
    transport_ = make_shared<MyTransport>(this,backlog_);
    threadPool_.reset(new MThreadPool("MainThreadPool"));
    threadPool_->setMaxQueueSize(40);
    threadPool_->start(POOL_SIZE);

    // 创建io子线程,并加入线程池中运行
    iothreads_.reserve(iothreadSize_);
    for(size_t i=0;i<iothreadSize_;i++){
        iothreads_.push_back(std::make_shared<IOThread>(this));
        threadPool_->run(std::bind(&IOThread::runInThread,iothreads_.back().get()));
    }

    redisPool = make_shared<RedisPool>(config_->redisConfig());
    redisPool->init();
    threadPool_->run(std::bind(&RedisPool::runInThread,redisPool.get()));

    // Mysql Pool
    auto mysqlPool = MysqlPool::getInstance();
    mysqlPool->setParameter("localhost","root","wanjun","test_db",3306,NULL,0,3);

    // threadPool_->run(std::bind(&MysqlPool::runInThread,mysqlPool));

    timerMgr_ = make_shared<TimerManager>();
    timerMgr_->init();

    threadPool_->run(std::bind(&TimerManager::runInThread,timerMgr_));

    // 添加需要解析的协议种类
    protocol_ = make_shared<MultipleProtocol>();
    protocol_->addProtocol(std::make_shared<ChatProtocol>());
    protocol_->addProtocol(std::make_shared<EchoProtocol>());

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

    LOG_DEBUG("Main server vector sockets size = [%lu]\n", activeTConnection.size());
    for (size_t i = 0; i < activeTConnection.size(); i++){
        delete activeTConnection[i];
    }

    LOG_DEBUG("Main server connection queue size = [%lu] \n", connectionQueue.size());
    while (!connectionQueue.empty()){
        TConnection *conn = connectionQueue.front();
        connectionQueue.pop();
        delete conn;
    }

    // 线程池手动调用结束,防止程序退出时死锁
    threadPool_->stop();
    event_free(ev_stdin);
    event_base_free(main_base);
}

void MainServer::serve(){
    LOG_DEBUG("serve() ... start \n");
    // 添加一个 控制台输入事件
    ev_stdin = event_new(main_base, STDIN_FILENO, EV_READ | EV_PERSIST, stdinCallBack, this);
    event_add(ev_stdin, NULL);

    transport_->listen(port_);

    event_base_dispatch(main_base);
    LOG_DEBUG("serve() ... end \n");
}

// server 处理 来自 transport 的 client_conn
void MainServer::handlerConn(void *args)
{
    // LOG_DEBUG("main server handler client_conn \n");

    MyTransport *transport = (MyTransport *)args;
    TSocket *sock = transport->accept();
    // LOG_DEBUG("sock->getSocketFD() = [%d] \n", sock->getSocketFD());

    // 将接受的 TSocket 包装成 TConnection ， 使用 main_server->eventbase
    TConnection *conn;
    {
        std::lock_guard<std::mutex> locker(connMutex);
        if (connectionQueue.empty()){
            ++selectIOThread_;
            selectIOThread_ = selectIOThread_ % IOTHREAD_SIZE;
            // LOG_DEBUG("select iothread num： [%d]\n",selectIOThread_);
            // 选择 iothread 创建 TConnection
            conn = new TConnection(sock, iothreads_[selectIOThread_].get());
            // LOG_DEBUG("新建一个 TConnection \n");
        }else{
            conn = connectionQueue.front();
            connectionQueue.pop();
            conn->setSocket(sock);
            // LOG_DEBUG("复用一个 TConnection \n");
        }

        if(conn){
            // 开启 connection 的 bufferevent
            conn->notify();
            activeTConnection.push_back(conn);
            // LOG_DEBUG("mainserver active push back conn\n");
        }else{
            LOG_DEBUG("socket --> TConnection 失败 ...\n");
        }
    }
}

void MainServer::returnTConnection(TConnection *conn){
    std::lock_guard<std::mutex> locker(connMutex);
    LOG_DEBUG("main server 回收 TConnection start ...\n");

    // 获得 TConnection 包装的 socket
    TSocket *sock = conn->getSocket();
    // 回收 sock
    transport_->returnTSocket(sock);
    // 回收 TConnection : stdLLvector 中的删除形式
    activeTConnection.erase(std::remove(activeTConnection.begin(),
                                        activeTConnection.end(), conn),
                            activeTConnection.end());
    // connectionQueue 中 conn 都带有一个关闭的 TSocket
    conn->setSocket(NULL);
    connectionQueue.push(conn);
}

bool MainServer::isActive(TConnection *conn) const{
    auto iter = find(activeTConnection.begin(),activeTConnection.end(),conn);
    if(iter!= activeTConnection.end()){
        return true;
    }else{
        return false;
    }
}

void MainServer::heartBeat(){
    for(auto conn : activeTConnection)
    {
        conn->heartBeat();
    }
}

void MainServer::execute(std::string cmd,MainServer *server){
    if (cmd == "list"){
        LOG_DEBUG("connection vector size %lu \n", server->activeTConnection.size());
        LOG_DEBUG("connection queue size %lu \n", server->connectionQueue.size());
        LOG_DEBUG("transport activeSokcet size = %d\n",server->transport_->getActiveSize());
        LOG_DEBUG("transport socketqueue size = %d\n\n",server->transport_->getSocketQueue());
    }else if(cmd == "hb"){
        LOG_DEBUG("heart beat\n");
        server->heartBeat();
    }else{
        LOG_DEBUG("cmd error...\n");
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