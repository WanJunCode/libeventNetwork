#include "MainServer.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>

typedef unsigned char BYTE;

MainServer::MainServer(int port)
    : port_(port)
{
    printf("main server structure ...\n");
    main_base = event_base_new();
    transport_ = new MyTransport(this);
}

struct event_base *MainServer::getBase()
{
    return main_base;
}

MainServer::~MainServer()
{
    event_base_free(main_base);
    printf("main server destructure ...\n");

    if (transport_ != NULL)
    {
        delete transport_;
    }
    transport_ = NULL;

    printf("Main server vector sockets size = [%lu]\n", activeTConnection.size());
    for (int i = 0; i < activeTConnection.size(); i++)
    {
        delete activeTConnection[i];
    }

    printf("Main server connection queue size = [%lu] \n", connectionQueue.size());
    while (!connectionQueue.empty())
    {
        TConnection *conn = connectionQueue.front();
        connectionQueue.pop();
        delete conn;
    }
}

void MainServer::serve()
{
    printf("serve() ... start \n");

    ev_stdin = event_new(main_base, STDIN_FILENO, EV_READ | EV_PERSIST, stdin_cb, this);
    event_add(ev_stdin, NULL);

    transport_->listen(12345);

    event_base_dispatch(main_base);

    printf("serve() ... end \n");
}

// server 处理 来自transport的 client_conn
void MainServer::handlerConn(void *args)
{
    printf("main server handler client_conn \n");

    MyTransport *transport = (MyTransport *)args;
    TSocket *sock = transport->accept();
    printf("sock->getSocketFD()  %d \n", sock->getSocketFD());

    // 将接受的 TSocket 包装成 TConnection ， 使用 main_server->eventbase
    TConnection *conn;
    if(connectionQueue.empty()){
        conn = new TConnection(sock, this);
        printf("新建一个 TConnection \n");
    }else{
        conn= connectionQueue.front();
        connectionQueue.pop();
        conn->setSocket(sock);
        printf("复用一个 TConnection \n");
    }

    if (conn)
    {
        conn->transition();
        activeTConnection.push_back(conn);
    }
}

void MainServer::returnTConnection(TConnection *conn)
{
    // 上锁
    printf("main server 回收 TConnection ...\n");

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
    printf("main server 回收 end\n");
}

void MainServer::stdin_cb(evutil_socket_t stdin_fd, short what, void *args)
{
    MainServer *server = (MainServer *)args;
    char recvline[2048];
    int len = read(stdin_fd, recvline, sizeof(recvline));
    recvline[len - 1] = '\0';
    printf("\nyou have input message : [%s] \n", recvline);
    if (strstr(recvline, "over") != NULL)
    {
        event_base_loopbreak(server->getBase());
    }
    else if (strstr(recvline, "size") != NULL)
    {
        printf("connection vector size %lu \n", server->activeTConnection.size());
        printf("connection queue size %lu \n", server->connectionQueue.size());

    }

}
