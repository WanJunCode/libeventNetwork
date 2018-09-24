#ifndef __TCONNECTION_H__
#define __TCONNECTION_H__

#include "TSocket.h"
#include "MainServer.h"

#include <event.h>

class TSocket;
class MainServer;

class TConnection
{
  public:
    explicit TConnection(TSocket *sock, MainServer *server);
    ~TConnection();
    void init();
    void transition();
    void setSocket(TSocket *socket);
    void close();
    MainServer *getServer();
    TSocket *getSocket();

  private:
    TSocket *underlying_socket;
    MainServer *server_;
    struct event_base *base_;
    struct bufferevent *bev;

  private:
    static void read_cb(struct bufferevent *bev, void *args);
    static void write_cb(struct bufferevent *bev, void *args);
    static void error_cb(struct bufferevent *bev, short what, void *args);
};

#endif