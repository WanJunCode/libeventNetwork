#include "MainServer.h"

#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <chrono>
#include "IOThread.h"
#include "Package/ChatPackage.h"

#define MAXBUFFERSIZE (1024*16*16)
typedef unsigned char BYTE;

MainServer::MainServer()
    :port_(PORT),
    selectIOThread(0),
    maxBufferSize(MAXBUFFERSIZE)            // 定义　bufferevent　最大的水位
{
    LOG_DEBUG("main server structure ...\n");
    main_base = event_base_new();
    transport_ = new MyTransport(this);
    thread_pools = new ThreadPool(POOL_SIZE);

    for(int i=0;i<IOTHREAD_SIZE;i++){
        // 使用智能指针，在析构函数中不再需要手动析构
        iothreads_.push_back(std::make_shared<IOThread>(this));
    }
    
    // 开启 iothread loop
    for(int i=0;i<IOTHREAD_SIZE;i++){
        thread_pools->enqueue(std::ref(*iothreads_[i]));
    }

    redis_pool = make_shared<RedisPool>("127.0.0.1",6379,"",100,5);
    thread_pools->enqueue(std::ref(*redis_pool));

    protocol = new MultipleProtocol();
    protocol->addProtocol(std::make_shared<ChatProtocol>());
    protocol->addProtocol(std::make_shared<EchoProtocol>());

}

MainServer::~MainServer()
{
    LOG_DEBUG("main server destructure ...\n");
    if (transport_ != NULL)
    {
        delete transport_;
        transport_ = NULL;
    }

    for(int i=0;i<iothreads_.size();i++){
        iothreads_[i]->breakLoop(false);
    }

    redis_pool->exit();

    if(thread_pools != NULL)
    {
        delete thread_pools;
        thread_pools = NULL;
    }

    LOG_DEBUG("Main server vector sockets size = [%lu]\n", activeTConnection.size());
    for (int i = 0; i < activeTConnection.size(); i++)
    {
        delete activeTConnection[i];
    }

    LOG_DEBUG("Main server connection queue size = [%lu] \n", connectionQueue.size());
    while (!connectionQueue.empty())
    {
        TConnection *conn = connectionQueue.front();
        connectionQueue.pop();
        delete conn;
    }

    delete protocol;
    event_free(ev_stdin);
    // 释放 mainserver 的 eventbase
    event_base_free(main_base);
}

void MainServer::serve()
{
    LOG_DEBUG("serve() ... start \n");
    // 添加一个 控制台输入事件
    ev_stdin = event_new(main_base, STDIN_FILENO, EV_READ | EV_PERSIST, stdin_cb, this);
    event_add(ev_stdin, NULL);

    transport_->listen(PORT);

    event_base_dispatch(main_base);
    LOG_DEBUG("serve() ... end \n");
}

// server 处理 来自 transport 的 client_conn
void MainServer::handlerConn(void *args)
{
    LOG_DEBUG("main server handler client_conn \n");

    MyTransport *transport = (MyTransport *)args;
    TSocket *sock = transport->accept();
    LOG_DEBUG("sock->getSocketFD()  %d \n", sock->getSocketFD());

    // 将接受的 TSocket 包装成 TConnection ， 使用 main_server->eventbase
    TConnection *conn;
    {
        std::lock_guard<std::mutex> locker(connMutex);
        if (connectionQueue.empty())
        {
            selectIOThread = selectIOThread % IOTHREAD_SIZE;
            LOG_DEBUG("select iothread num： [%d]\n",selectIOThread++);

            // 选择 iothread 创建 TConnection
            conn = new TConnection(sock, iothreads_[selectIOThread].get());
            LOG_DEBUG("新建一个 TConnection \n");
        }
        else
        {
            conn = connectionQueue.front();
            connectionQueue.pop();
            conn->setSocket(sock);
            LOG_DEBUG("复用一个 TConnection \n");
        }

        if (conn)
        {
            // 开启 connection 的 bufferevent
            conn->notify();
            activeTConnection.push_back(conn);
            LOG_DEBUG("mainserver active push back conn\n");
        }
        else
        {
            LOG_DEBUG("socket --> TConnection 失败 ...\n");
        }
    }
}

void MainServer::returnTConnection(TConnection *conn)
{
    std::lock_guard<std::mutex> locker(connMutex);
    // 上锁
    LOG_DEBUG("main server 回收 TConnection start ...\n");

    // 获得 TConnection 包装的 socket
    TSocket *sock = conn->getSocket();
    // 回收 sock
    transport_->returnTSocket(sock);

    // 回收 TConnection
    activeTConnection.erase(std::remove(activeTConnection.begin(),
                                        activeTConnection.end(), conn),
                            activeTConnection.end());
    // connectionQueue 中 conn 都带有一个关闭的 TSocket
    conn->setSocket(NULL);
    connectionQueue.push(conn);
    LOG_DEBUG("main server 回收 TConnection end ...\n");
}

struct event_base *MainServer::getBase()
{
    return main_base;
}

ThreadPool *MainServer::getPool(){
    return thread_pools;
}


void MainServer::stdin_cb(evutil_socket_t stdin_fd, short what, void *args)
{
    MainServer *server = static_cast<MainServer *>(args);
    char recvline[80];
    int len = read(stdin_fd, recvline, sizeof(recvline));
    recvline[len - 1] = '\0';
    LOG_DEBUG("you have input cmd : [%s] \n", recvline);
    if (strstr(recvline, "over") != NULL)
    {
        event_base_loopbreak(server->getBase());
    }
    else if (strstr(recvline, "size") != NULL)
    {
        LOG_DEBUG("connection vector size %lu \n", server->activeTConnection.size());
        LOG_DEBUG("connection queue size %lu \n", server->connectionQueue.size());
        LOG_DEBUG("transport activeSokcet size = %d\n",server->transport_->getActiveSize());
        LOG_DEBUG("transport socketqueue size = %d\n\n\n",server->transport_->getSocketQueue());
    }
    else if(strstr(recvline,"thread")!=NULL)
    {
        // server->getPool()->enqueue(std::ref(*(server->io)));
        // LOG_DEBUG("cmd error...\n");
    }
    else
    {
        LOG_DEBUG("cmd error...\n");
    }
    // test thread pool
}

bool MainServer::isActive(TConnection *conn)
{
    auto iter = find(activeTConnection.begin(),activeTConnection.end(),conn);
    if(iter!= activeTConnection.end()){
        return true;
    }else{
        return false;
    }
}