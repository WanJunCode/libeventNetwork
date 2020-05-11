#include "IOThread.h"
#include "logcpp.h"

#include "TSocket.h"
#include "PortListener.h"
#include "TConnection.h"
#include "MainServer.h"

#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <string.h>
#include <event2/event.h>
#include <sys/select.h>
#include <assert.h>

#ifndef SOCKOPT_CAST_T
#ifndef _WIN32
#define SOCKOPT_CAST_T void         // _WIN32
#else
#define SOCKOPT_CAST_T char         // _Linux
#endif
#endif

template <class T>
inline const SOCKOPT_CAST_T* const_cast_sockopt(const T* v) {
    return reinterpret_cast<const SOCKOPT_CAST_T*>(v);
}

template <class T>
inline SOCKOPT_CAST_T* cast_sockopt(T* v) {
    return reinterpret_cast<SOCKOPT_CAST_T*>(v);
}

IOThread::IOThread(MainServer *server):
    base_(NULL),
    server_(server)
{
    memset(notificationPipeFDs_, -1, sizeof(notificationPipeFDs_));
}

IOThread::~IOThread()
{
    // 退出 base
    event_base_loopbreak(base_);

    if(base_!= NULL){
        event_base_free(base_);
    }

    //关闭与主线程通信的pipe
    for (int i = 0; i < 2; ++i) {
        if (notificationPipeFDs_[i] >= 0) {
            if (0 != EVUTIL_CLOSESOCKET(notificationPipeFDs_[i])) {
                LOG_ERROR("TNonblockingIOThread notificationPipe close() ERROR");
            }
            notificationPipeFDs_[i] = INVALID_SOCKET;
        }
    }
}

void IOThread::runInThread()
{
    threadId_ = pthread_self();

    if(base_ == NULL){
        base_ = event_base_new();// 在新的线程上创建一个 eventbase
    }
    // 创建 notification 管道
    createNotificationPipe();

    // 为接受管道设置一个读取事件 notificationEvent_, 只要管道中接收到数据便会回调
    event_set(&notificationEvent_, getNotificationRecvFD(), EV_READ | EV_PERSIST, IOThread::notifyHandler, this);

    // Attach to the base
    event_base_set(base_, &notificationEvent_);

    // if (-1 == event_add(&notificationEvent_, 0)) {
    if (-1 == event_add(&notificationEvent_, NULL)) {
        // time is NULL means wait forever
        LOG_DEBUG("TNonblockingServer::serve(): event_add() failed on task-done notification event");
    }

    // 开启线程中的事件循环
    event_base_dispatch(base_);
}

void IOThread::createNotificationPipe()
{
    if(evutil_socketpair(AF_LOCAL,SOCK_STREAM,0,notificationPipeFDs_) == -1)
    {
        LOG_DEBUG("can't create notification pipe, error:[%d]\n",EVUTIL_SOCKET_ERROR());
        return;
    }
    // 读写管道设置为非阻塞模式，总是可读可写
    if(evutil_make_socket_nonblocking(notificationPipeFDs_[0]) < 0
        || evutil_make_socket_nonblocking(notificationPipeFDs_[1]) < 0 )
    {
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[0]);
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[1]);
            LOG_ERROR("TNonblockingServer::createNotificationPipe() THRIFT_O_NONBLOCK\n");
            return;
    }

    for (int i = 0; i < 2; ++i) {
        // 设置当调用exec()会关闭指定的套接字
        if (evutil_make_socket_closeonexec(notificationPipeFDs_[i]) < 0) {
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[0]);
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[1]);
            LOG_ERROR("TNonblockingServer::createNotificationPipe() FD_CLOEXEC");
            return;
        }
    }
}

// 退出IO Thread的事件循环
void IOThread::breakLoop(bool error) {
    if (error) {
        LOG_DEBUG("TNonblockingServer: IO thread #%d exiting with error.\n", threadId_);

        // TODO: figure out something better to do here, but for now kill the
        // whole process.
        LOG_DEBUG("TNonblockingServer: aborting process.\n");
        ::abort();
    }

    // If we're running in the same thread, we can't use the notify(0)
    // mechanism to stop the thread, but happily if we're running in the
    // same thread, this means the thread can't be blocking in the event
    // loop either.
    if (!pthread_equal(pthread_self(), threadId_)) {
        // 其他线程通过notify关闭该 IOThread
        notify(NULL);
    } else {
        // cause the loop to stop ASAP - even if it has things to do in it
        event_base_loopbreak(base_);
    }
}


// 在 IOThread 线程中激活一个连接
// 将参数 TConnection* conn 指针地址通过管道发送出去
bool IOThread::notify(TConnection* conn) {
    evutil_socket_t fd = getNotificationSendFD();
    if (fd < 0) {
        return false;
    }

    fd_set wfds, efds;
    long ret = -1;
    long kSize = sizeof(conn);
    // make TConnection* conn ==> const char* pos 
    const char* pos = reinterpret_cast<const char*>(&conn);

    while (kSize > 0) {
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        FD_SET(fd, &wfds);
        FD_SET(fd, &efds);
        // int select(int nfds,  fd_set* readset,  fd_set* writeset,  fe_set* exceptset,  struct timeval* timeout);
        // (fd + 1) make sure select() will find the event happen on fd
        // read   --  null
        // write  --  wfds
        // except --  efds
        // timeout -- null is blocked . 0 is nonblocked
        // the write pipe is always writeable, use this function to test write pipe fd is OK
        ret = select(static_cast<int>(fd + 1), NULL, &wfds, &efds, NULL);
        if (ret < 0) {
            return false;
        } else if (ret == 0) {
            continue;
        }

        // if any except happen
        if (FD_ISSET(fd, &efds)) {
            EVUTIL_CLOSESOCKET(fd);
            LOG_ERROR("Thread [%d] write pipe fd is except happen\n",pthread_self());
            return false;
        }

        // 写管道是非阻塞的，总是可写
        if (FD_ISSET(fd, &wfds)) {
            LOG_DEBUG("Thread [%d] send TConnection from write pipe where address at [%x]\n",pthread_self(),pos);
            // 通过IOThread 的fd 管道发送关闭 socket 信号
            ret = send(fd, pos, kSize, 0);
            if (ret < 0) {
                if (errno == EAGAIN) {
                    continue;
                }
                EVUTIL_CLOSESOCKET(fd);
                return false;
            }
            kSize -= ret;
            pos += ret;
        }
    }

    return true;
}


// 从管道中读取 TConnection *指针并启动 bufferevent
// 如果读取到的是 NULL ，则关闭该 IOThread
/* static */
void IOThread::notifyHandler(evutil_socket_t fd, short which, void* v) {
    IOThread* ioThread = (IOThread*)v;
    assert(ioThread);
    (void)which;

    do {
        TConnection* connection = NULL;
        const int kSize = sizeof(connection);

        // 将接收到的数据 写入TConnection*指针指向的地址
        long nBytes = recv(fd, reinterpret_cast<char *>(&connection), kSize, 0);
        if (nBytes == kSize) {
            if (NULL == connection) {
                // this is the command to stop our thread, exit the handler!
                LOG_WARN("thread [%d] read NULL from read pipe to terminate itself\n",pthread_self());
                ioThread->breakLoop(false);
                return;
            }
            // TODO 接收到一个完整的 TConnection
            if (ioThread->getServer()->isActive(connection)) {
                LOG_DEBUG("thread [%d] read TConnection from read pipe address at [%x]\n",pthread_self(),connection);
                // 在此执行关闭
                // connection->transition();
            }
            connection->transition();

        } else if (nBytes > 0) {
            // throw away these bytes and hope that next time we get a solid read
            LOG_WARN("notifyHandler: Bad read of %ld bytes, wanted %d\n", nBytes, kSize);
            ioThread->breakLoop(true);
            return;
        } else if (nBytes == 0) {
            LOG_WARN("notifyHandler: Notify socket closed!\n");
            ioThread->breakLoop(false);
            // exit the loop
            break;
        } else { // nBytes < 0
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                LOG_WARN("TNonblocking: notifyHandler read() failed: [%d]\n",EVUTIL_SOCKET_ERROR());
                ioThread->breakLoop(true);
                return;
            }
            // exit the loop
            break;
        }
        
        // already transition one connection
    } while (true);
}
