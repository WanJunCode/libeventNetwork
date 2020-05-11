#ifndef __WJ_PortListener_H__
#define __WJ_PortListener_H__

#include <event2/listener.h>

class MainServer;
class PortListener{
public:
    PortListener(size_t backlog = 10);
    ~PortListener();
    void listen(MainServer *server, int port);

public:
    static void do_accept(struct evconnlistener *listener, evutil_socket_t client_fd, struct sockaddr *addr, int socklen, void *args);

private:
    struct evconnlistener *listener_;
    size_t backlog_;
};

#endif