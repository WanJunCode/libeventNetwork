#ifndef __WJ_MYTRANSPORT_H__
#define __WJ_MYTRANSPORT_H__

#include "MainServer.h"
#include "TSocket.h"

#include <event2/listener.h>
#include <event.h>
#include <map>
#include <queue>
#include <mutex>

class MainServer;
class TSocket;

class MyTransport
{
  public:
    explicit MyTransport(MainServer *server);
    ~MyTransport();
    void listen(int port);
    TSocket *accept();
    void returnTSocket(TSocket *sock);
    int getActiveSize();
    int getSocketQueue();

  public:
    static void do_accept(struct evconnlistener *listener, evutil_socket_t client_fd, struct sockaddr *addr, int socklen, void *args);

  private:
    MainServer *server_;
    struct evconnlistener *listener_;
    struct event_base *main_base_;
    std::mutex connMutex_;

    std::map<evutil_socket_t, TSocket *> activeSocket;
    std::queue<TSocket *> socketQueue;
};

#endif