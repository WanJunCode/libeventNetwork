#ifndef __WJ_PortListener_H__
#define __WJ_PortListener_H__

#include <event2/listener.h>
#include <event.h>
#include <map>
#include <queue>
#include <mutex>

class MainServer;
class TSocket;

class PortListener{
public:
    explicit PortListener(size_t backlog = 10);
    ~PortListener();
    void listen(MainServer *server, int port);
    void returnTSocket(TSocket *sock);
    int getActiveSize();
    int getSocketQueue();
    TSocket *ReuseTSocket(evutil_socket_t client_fd);

public:
    static void do_accept(struct evconnlistener *listener, evutil_socket_t client_fd, struct sockaddr *addr, int socklen, void *args);

private:
    struct evconnlistener *listener_;
    std::mutex connMutex_;
    size_t backlog_;
    std::map<evutil_socket_t, TSocket *> activeSocket;
    std::queue<TSocket *> socketQueue;
};

#endif