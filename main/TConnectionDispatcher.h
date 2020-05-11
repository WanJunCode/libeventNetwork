#pragma once

#include <mutex>

#include "../base/noncopyable.h"
#include "MainServer.h"
#include "TConnection.h"


class TConnectionDispatcher : public noncopyable {
public:
    TConnectionDispatcher(MainServer *server)
    :mainServer(server){
    }

    ~TConnectionDispatcher();

    void handleConn(evutil_socket_t client);
    void returnTConnection(TConnection *conn);
    bool isActive(TConnection *conn);
    void heartBeat();

    std::string debug();

private:
    TSocket *setSocket(evutil_socket_t client);
    void setConnection(TSocket *sock);
    
    MainServer *mainServer;
    std::mutex sockMutex_;
    std::mutex connMutex_;
    // 活动的连接
    std::map<evutil_socket_t,TConnection *> activeTConnectionVector;
    std::queue<TConnection *> ReuseConnectionQueue;
    std::map<evutil_socket_t, TSocket *> activeSocket;
    std::queue<TSocket *> reuseSocketQueue;
};