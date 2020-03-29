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

    std::ostringstream oss;
    oss<<"from :Cloud Server"<<endl;
    oss<<"active TConnection size ="<<activeTConnectionVector.size()<<endl;
    oss<<"reuseable TConnection size ="<<ReuseConnectionQueue.size()<<endl;
    oss<<"active TSocket size = "<<listener_->getActiveSize()<<endl;
    oss<<"reuseable TSocket size = "<<listener_->getSocketQueue()<<endl;
    oss<<MysqlPool::getInstance()->debug();
    resp->setBody(oss.str());
}

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

MainServer::~MainServer(){
    LOG_DEBUG("main server destructure ...\n");

    for(size_t i=0;i<iothreads_.size();i++){
        iothreads_[i]->breakLoop(false);
    }

    if(redisPool)
        redisPool->exit();
    
    if(timerMgr_)
        timerMgr_->stop();

    LOG_DEBUG("Main server vector sockets size = [%lu]\n", activeTConnectionVector.size());
    for (size_t i = 0; i < activeTConnectionVector.size(); i++){
        ReuseConnectionQueue.push(activeTConnectionVector[i]);
    }

    LOG_DEBUG("Main server connection queue size = [%lu] \n", ReuseConnectionQueue.size());
    while (!ReuseConnectionQueue.empty()){
        TConnection *conn = ReuseConnectionQueue.front();
        ReuseConnectionQueue.pop();
        delete conn;
    }

    // 线程池手动调用结束,防止程序退出时死锁
    threadPool_->stop();
    event_free(ev_stdin);
    event_base_free(main_base);
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
void MainServer::handlerConn(void *args)
{
    TSocket *sock = (TSocket *)args;
    // LOG_DEBUG("main server handler client_conn \n");
    // LOG_DEBUG("sock->getSocketFD() = [%d] \n", sock->getSocketFD());
    // 将接受的 TSocket 包装成 TConnection ， 使用 main_server->eventbase
    TConnection *conn;
    {
        std::lock_guard<std::mutex> locker(connMutex);
        if (ReuseConnectionQueue.empty()){
            ++selectIOThread_;
            // Load balancing  负载均衡
            selectIOThread_ = selectIOThread_ % IOTHREAD_SIZE;
            // 选择 iothread 创建 TConnection
            conn = new TConnection(sock, iothreads_[selectIOThread_].get());
        }else{
            conn = ReuseConnectionQueue.front();
            ReuseConnectionQueue.pop();
            conn->setSocket(sock);
        }

        if(conn){
            // 开启 connection 的 bufferevent
            conn->notify();
            // 所有的TConnection 由MainServer管理，但是分派到不同的线程中进行数据读取与发送
            activeTConnectionVector.push_back(conn);
        }else{
            LOG_DEBUG("socket --> TConnection 失败 ...\n");
        }
    }
}

// TODO 
void MainServer::returnTConnection(TConnection *conn){
    std::lock_guard<std::mutex> locker(connMutex);
    LOG_DEBUG("main server 回收 TConnection start ...\n");

    // 获得 TConnection 包装的 socket
    TSocket *sock = conn->getSocket();
    // 回收 sock
    listener_->returnTSocket(sock);
    // 回收 TConnection : std::vector 中的删除形式
    activeTConnectionVector.erase(std::remove(activeTConnectionVector.begin(),activeTConnectionVector.end(), conn),
                            activeTConnectionVector.end());

    // ReuseConnectionQueue 中 conn 都带有一个关闭的 TSocket
    // 断开 TConnection 与 TSocket 之间的关系
    conn->setSocket(NULL);
    ReuseConnectionQueue.push(conn);
}

bool MainServer::isActive(TConnection *conn) const{
    auto iter = find(activeTConnectionVector.begin(),activeTConnectionVector.end(),conn);
    if(iter!= activeTConnectionVector.end()){
        return true;
    }else{
        return false;
    }
}

void MainServer::heartBeat(){
    for(auto conn : activeTConnectionVector)
    {
        conn->heartBeat();
    }
}

void MainServer::execute(std::string cmd,MainServer *server){
    if (cmd == "list"){
        LOG_DEBUG("MainServer 激活的 TConnection size %lu\n", server->activeTConnectionVector.size());
        LOG_DEBUG("MainServer 可复用的 TConnection size %lu\n", server->ReuseConnectionQueue.size());
        LOG_DEBUG("PortListener 激活的 TSocket size = %d\n",server->listener_->getActiveSize());
        LOG_DEBUG("PortListener 可复用的 TSocket size = %d\n",server->listener_->getSocketQueue());
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

