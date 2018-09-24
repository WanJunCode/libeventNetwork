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

MyTransport::MyTransport(MainServer *server)
    : server_(server)
{
    printf("my transport structure ...\n");
    main_base_ = server->getBase();
}

MyTransport::~MyTransport()
{
    printf("my transport destructure ...\n");

    if (listener_ != NULL)
    {
        evconnlistener_free(listener_);
        listener_ = NULL;
    }

    printf("my transport size = %lu\n", socketQueue.size());
    while (!socketQueue.empty())
    {
        printf("transport socket queue pop...\n");
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

    printf("新建 listener ...\n");
    listener_ = evconnlistener_new_bind(main_base_, do_accept, this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 10, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (!listener_)
    {
        perror("listener 新建 失败 ...\n");
        assert(0);
    }

    evutil_socket_t listener_fd = evconnlistener_get_fd(listener_);
    evutil_make_socket_nonblocking(listener_fd);
    evutil_make_listen_socket_reuseable(listener_fd);
}

TSocket *MyTransport::accept()
{
    TSocket *sock = socketQueue.front();
    socketQueue.pop();
    return sock;
}

void MyTransport::do_accept(struct evconnlistener *listener, evutil_socket_t client_fd,
                            struct sockaddr *addr, int socklen, void *args)
{
    printf("新的 client 连接 %d...\n", client_fd);
    MyTransport *transport = (MyTransport *)args;

    {
        // 上锁
        std::lock_guard<std::mutex>(transport->connMutex_);
        auto iter = transport->activeSocket.find(client_fd);
        if (iter != transport->activeSocket.end())
        {
            perror("client 已经处于 active 状态 ...\n");
            transport->activeSocket.erase(iter);
        }

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
    // args -> this
    transport->server_->handlerConn(args);
}

void MyTransport::returnTSocket(TSocket *sock)
{
    printf("回收 TSocket \n");
    // 上锁
    auto iter = activeSocket.find(sock->getSocketFD());
    if (iter != activeSocket.end())
    {
        printf("active socket map erase sock \n");
        activeSocket.erase(iter);
    }
    // 关闭回收的TSocket连接
    sock->close();
    socketQueue.push(sock);
}
