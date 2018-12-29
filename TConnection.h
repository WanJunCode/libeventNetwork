#ifndef __TCONNECTION_H__
#define __TCONNECTION_H__

#include "TSocket.h"
#include "MainServer.h"

#include <event.h>

class TSocket;
class MainServer;
class IOThread;

// TConnection 只负责 TSocket 的快速启用 bufferevent
// 析构函数中并不会关闭传入的 TSocket
// evebtbase 来自 MainServer 或者 IOThread
class TConnection
{
public:
    explicit TConnection(TSocket *sock, MainServer *server);
    explicit TConnection(TSocket *sock, IOThread *iothread);

    ~TConnection();
    
    void init();
    void transition();
    void setSocket(TSocket *socket);
    void close();
    MainServer *getServer();
    TSocket *getSocket();
    bool notify();

private:
    TSocket *socket_;
    MainServer *server_;
    IOThread *iothread_;
    struct event_base *base_;
    struct bufferevent *bev;

private:
    static void read_cb(struct bufferevent *bev, void *args);
    static void write_cb(struct bufferevent *bev, void *args);
    static void error_cb(struct bufferevent *bev, short what, void *args);
};

#endif