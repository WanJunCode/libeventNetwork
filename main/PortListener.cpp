#include "PortListener.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <event.h>
#include <event2/listener.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <memory>

#include "MainServer.h"
#include "TSocket.h"
#include "logcpp.h"

PortListener::PortListener(size_t backlog)
    : backlog_(backlog){
    LOG_DEBUG("initialize listener\n");
}

PortListener::~PortListener(){
    if (listener_ != NULL){
        evconnlistener_free(listener_);
        listener_ = NULL;
    }

    // 清除  active map

    // 清除  socket queue
    LOG_DEBUG("listener size = [%lu]\n", socketQueue.size());
    while (!socketQueue.empty()){
        TSocket *tmp = socketQueue.front();
        socketQueue.pop();
        delete tmp;
    }
}

// TODO listen函数从外部获得 struct event_base 用于注册监听回调函数
void PortListener::listen(MainServer *server, int port){
    struct sockaddr_in bindaddr;
    bzero(&bindaddr, sizeof(bindaddr));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(port);

    // reuseable  &&  close_on_free
    listener_ = evconnlistener_new_bind(server->getBase(), do_accept, server, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 
        backlog_, (struct sockaddr *)&bindaddr, sizeof(bindaddr));

    if (!listener_){
        perror("create listener error...\n");
        assert(false);
    }

    // 设置 listen fd 为 nonblocking 
    if(0 != evutil_make_socket_nonblocking(evconnlistener_get_fd(listener_))){
        LOG_DEBUG("faliure to make listener socket nonblocking\n");
    }
    // evutil_make_listen_socket_reuseable(listener_fd);
}

TSocket *PortListener::ReuseTSocket(evutil_socket_t client_fd){
#if 0
    // 判断客户端文件描述符是否已经存在activeSocket map中
    auto iter = activeSocket.find(client_fd);
    if (iter != activeSocket.end()){
        perror("client 已经处于 active 状态 ...\n");
        activeSocket.erase(iter);
    }
#endif
    // !! client fd 设置为非阻塞
    evutil_make_socket_nonblocking(client_fd);
    TSocket *sock = NULL;
    // RAII 最佳实践 (保护资源 )
    std::lock_guard<std::mutex> locker(connMutex_);
    // 复用一个 TSocket 完成对 client fd 的封装
    if (socketQueue.empty()){
        // 没有可复用的则创建新的 TSocket
        sock = new TSocket(client_fd);
        socketQueue.push(sock);
    }else{
        // 在此处将最前端的TSocket复用，后面会 pop 弹出
        sock = socketQueue.front();
        sock->setSocketFD(client_fd);
        socketQueue.pop();
    }
    activeSocket.insert(std::pair<evutil_socket_t, TSocket *>(client_fd, sock));
    return sock;
}

// 监听回调函数
// static
void PortListener::do_accept(struct evconnlistener *listener, evutil_socket_t client_fd,
                            struct sockaddr *addr, int socklen, void *args){
    UNUSED(listener);
    UNUSED(addr);
    UNUSED(socklen);
    // LOG_DEBUG("new client connection [%d]...\n", client_fd);
    MainServer *server = (MainServer *)args;
    std::shared_ptr<PortListener> listener = server->getListener();
    TSocket *sock = listener_->ReuseTSocket(client_fd);
    server->handlerConn(sock);
}

void PortListener::returnTSocket(TSocket *sock){
    LOG_DEBUG("listener return TSocket \n");
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
        
        sock->close();

        // 关闭回收的TSocket连接
        // delete sock;
        socketQueue.push(sock);
    }
}

int PortListener::getActiveSize(){
    return activeSocket.size();
}

int PortListener::getSocketQueue(){
    return socketQueue.size();
}