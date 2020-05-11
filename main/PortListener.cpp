#include "PortListener.h"
#include "MainServer.h"
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
    LOG_DEBUG("PortListener dtor...\n");
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

// 监听回调函数
// static
void PortListener::do_accept(struct evconnlistener *listener, evutil_socket_t client_fd,
                            struct sockaddr *addr, int socklen, void *args){
    UNUSED(listener);
    UNUSED(addr);
    UNUSED(socklen);
    LOG_DEBUG("new client connection [%d]...\n", client_fd);
    MainServer *server = (MainServer *)args;
    server->handlerConn(client_fd);
}