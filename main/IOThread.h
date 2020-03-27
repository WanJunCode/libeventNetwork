#ifndef WANJUN_IO_THREAD_H
#define WANJUN_IO_THREAD_H

#include <event2/listener.h>
#include <event.h>
#include <pthread.h>

class MainServer;
class TConnection;
class IOThread{
public:

    IOThread(MainServer *server);
    ~IOThread();

    // 创建与主线程通信的管道pipe
    void createNotificationPipe();
    // 线程执行函数
    void runInThread();

    // Returns the send-fd for task complete notifications.
    evutil_socket_t getNotificationSendFD() const {
        return notificationPipeFDs_[1];
    }
    // Returns the read-fd for task complete notifications.
    evutil_socket_t getNotificationRecvFD() const {
        return notificationPipeFDs_[0];
    }
    // return the connected main server
    MainServer *getServer(){
        return server_;
    }
    // return the event base on IO Thread
    struct event_base *getBase(){
        return base_;
    }

    // main thread revoke this function to send conn to IO Thread
    // send() TConnection* conn as const char *pos
    bool notify(TConnection* conn);

    // main thread revoke this function to terminate IO Thread
    void breakLoop(bool error);

private:
    static void notifyHandler(evutil_socket_t fd, short which, void* v);

    struct event_base *base_;
    MainServer *server_;
    unsigned long threadId_;
    evutil_socket_t notificationPipeFDs_[2];
    struct event notificationEvent_;            // 管道通知事件
};


#endif // !WANJUN_IO_THREAD_H

