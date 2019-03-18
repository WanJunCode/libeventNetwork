#ifndef __TCONNECTION_H__
#define __TCONNECTION_H__

#include "TSocket.h"
#include "MainServer.h"

#include <event.h>

class TSocket;
class MainServer;
class IOThread;

/// Three states for sockets: recv frame size, recv data, and send mode
enum class SocketState { SOCKET_RECV_FRAMING, SOCKET_RECV, SOCKET_SEND };

/**
* Five states for the nonblocking server:
*  1) initialize
*  2) read 4 byte frame size
*  3) read frame of data
*  4) send back data (if any)
*  5) force immediate connection close
*/
enum class AppState {
    TRANS_INIT,
    APP_INIT,
    APP_READ_FRAME_SIZE,
    APP_READ_REQUEST,
    APP_WAIT_TASK,
    APP_SEND_RESULT,
    APP_CLOSE_CONNECTION
};

// TConnection 只负责 TSocket 的快速启用 bufferevent
// 析构函数中并不会关闭传入的 TSocket
// evebtbase 来自 MainServer 或者 IOThread
class ChatPackage;
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
    void heartBeat();
    void record(std::string message);

    void workSocket();
    void recv_framing();
    void recv();

    void chishenme(std::string msg);

    void setHeartBeat(int heart){
        heartBeat_ = heart;
    }

    int getHeartBeat(){
        return heartBeat_;
    }

private:
    void read_request();
public:
    time_t lastUpdate_;
    int heartBeat_;

private:
    TSocket *socket_;
    MainServer *server_;
    IOThread *iothread_;
    struct event_base *base_;
    struct bufferevent *bev;
    AppState appstate;
    SocketState socketState;
    /// How much data needed to read
    size_t readWant_;
    /// Read buffer
    size_t frameSize_;
    size_t maxBufferSize_;

private:
    static void read_cb(struct bufferevent *bev, void *args);
    static void write_cb(struct bufferevent *bev, void *args);
    static void error_cb(struct bufferevent *bev, short what, void *args);
};

#endif