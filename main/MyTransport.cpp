#include "MyTransport.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <event.h>
#include <event2/listener.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

#include "MainServer.h"
#include "TSocket.h"
#include "logcpp.h"

MyTransport::MyTransport(MainServer *server,size_t backlog)
    : server_(server),
     backlog_(backlog)
{
    LOG_DEBUG("initialize transport\n");
}

MyTransport::~MyTransport()
{
    if (listener_ != NULL)
    {
        evconnlistener_free(listener_);
        listener_ = NULL;
    }

    // 清除  active map

    // 清除  socket queue
    LOG_DEBUG("transport size = [%lu]\n", socketQueue.size());
    while (!socketQueue.empty())
    {
        TSocket *tmp = socketQueue.front();
        socketQueue.pop();
        delete tmp;
    }
}

void MyTransport::listen(int port)
{
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // reuseable  &&  close_on_free
    listener_ = evconnlistener_new_bind(server_->getBase(), do_accept, this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 
        backlog_, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (!listener_){
        perror("create listener error...\n");
        assert(0);
    }

    // 设置 listen fd 为 nonblocking 
    if(0 != evutil_make_socket_nonblocking(evconnlistener_get_fd(listener_))){
        LOG_DEBUG("faliure to make listener socket nonblocking\n");
    }
    // evutil_make_listen_socket_reuseable(listener_fd);
}

TSocket *MyTransport::accept()
{
    std::lock_guard<std::mutex> locker(connMutex_);
    TSocket *sock = socketQueue.front();
    socketQueue.pop();
    return sock;
}

// static
void MyTransport::do_accept(struct evconnlistener *listener, evutil_socket_t client_fd,
                            struct sockaddr *addr, int socklen, void *args)
{
    UNUSED(listener);
    UNUSED(addr);
    UNUSED(socklen);
    // LOG_DEBUG("new client connection [%d]...\n", client_fd);
    MyTransport *transport = (MyTransport *)args;
    {
        std::lock_guard<std::mutex> locker(transport->connMutex_);

        auto iter = transport->activeSocket.find(client_fd);
        if (iter != transport->activeSocket.end())
        {
            perror("client 已经处于 active 状态 ...\n");
            transport->activeSocket.erase(iter);
        }

        // client fd 设置为 非阻塞
        evutil_make_socket_nonblocking(client_fd);
        if (transport->socketQueue.empty())
        {
            // 为空 新建 TSocket
            TSocket *sock = new TSocket(client_fd);
            transport->socketQueue.push(sock);
        }
        else
        {
            transport->socketQueue.front()->setSocketFD(client_fd);
        }
        transport->activeSocket.insert(std::pair<evutil_socket_t, TSocket *>(client_fd, transport->socketQueue.front()));
    }
    // args -> this, 主服务器 接收处理新建的 client
    transport->server_->handlerConn(args);
}

void MyTransport::returnTSocket(TSocket *sock)
{
    LOG_DEBUG("transport return TSocket \n");
    {
        // 从 active map 中删除
        std::lock_guard<std::mutex> locker(connMutex_);
        auto iter = activeSocket.find(sock->getSocketFD());
        if (iter != activeSocket.end()){
            LOG_DEBUG("active socket map erase sock [%d]\n",iter->first);
            activeSocket.erase(iter);
        }else{
            LOG_DEBUG("activeSocket 中没找到...\n");
        }
        
        // 关闭回收的TSocket连接
        sock->close();
        socketQueue.push(sock);
    }
}

int MyTransport::getActiveSize()
{
    return activeSocket.size();
}

int MyTransport::getSocketQueue()
{
    return socketQueue.size();
}
