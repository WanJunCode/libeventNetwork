#include "IOThread.h"
#include "logcpp.h"

#include "TSocket.h"
#include "MyTransport.h"
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

    for (int i = 0; i < 2; ++i) {
        if (notificationPipeFDs_[i] >= 0) {
            if (0 != EVUTIL_CLOSESOCKET(notificationPipeFDs_[i])) {
                LOG_DEBUG("TNonblockingIOThread notificationPipe close(): ");
            }
            notificationPipeFDs_[i] = INVALID_SOCKET;
        }
    }
}

void IOThread::start()
{
    threadId_ = pthread_self();

    if(base_ == NULL){
        // 在新的线程上创建一个 eventbase
        base_ = event_base_new();
    }
    // 创建 notification 管道
    createNotificationPipe();

    // 为接受管道设置一个 读取事件 notificationEvent_
    event_set(&notificationEvent_,
          getNotificationRecvFD(),
          EV_READ | EV_PERSIST,
          IOThread::notifyHandler,
          this);

    // Attach to the base
    event_base_set(base_, &notificationEvent_);

    // Add the event and start up the server
    // if (-1 == event_add(&notificationEvent_, 0)) {
    if (-1 == event_add(&notificationEvent_, NULL)) {
        // time is NULL means wait forever
        LOG_DEBUG("TNonblockingServer::serve(): event_add() failed on task-done notification event");
    }
}

void IOThread::runInThread()
{
    start();
    event_base_dispatch(base_);
}

void IOThread::createNotificationPipe()
{
    if(evutil_socketpair(AF_LOCAL,SOCK_STREAM,0,notificationPipeFDs_) == -1)
    {
        LOG_DEBUG("can't create notification pipe, error:[%d]\n",EVUTIL_SOCKET_ERROR());
        return;
    }
    if(evutil_make_socket_nonblocking(notificationPipeFDs_[0]) < 0
        || evutil_make_socket_nonblocking(notificationPipeFDs_[1]) < 0 )
    {
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[0]);
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[1]);
            LOG_DEBUG("TNonblockingServer::createNotificationPipe() THRIFT_O_NONBLOCK\n");
            return;
    }

    for (int i = 0; i < 2; ++i) {
        if (evutil_make_socket_closeonexec(notificationPipeFDs_[i]) < 0) {
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[0]);
            EVUTIL_CLOSESOCKET(notificationPipeFDs_[1]);
            LOG_DEBUG("TNonblockingServer::createNotificationPipe() FD_CLOEXEC");
            return;
        }
    }
}

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
        notify(NULL);
    } else {
        // cause the loop to stop ASAP - even if it has things to do in it
        event_base_loopbreak(base_);
    }
}

// 从管道中读取 TConnection 并启动 bufferevent
/* static */
void IOThread::notifyHandler(evutil_socket_t fd, short which, void* v) {
    IOThread* ioThread = (IOThread*)v;
    assert(ioThread);
    (void)which;

    do {
        TConnection* connection = NULL;
        const int kSize = sizeof(connection);

        // 将接收到的数据  写入connection
        long nBytes = recv(fd, cast_sockopt(&connection), kSize, 0);
        if (nBytes == kSize) {
            if (NULL == connection) {
                // this is the command to stop our thread, exit the handler!
                ioThread->breakLoop(false);
                return;
            }

            // 接收到一个完整的 TConnection
            if (ioThread->getServer()->isActive(connection)) {
                connection->transition();
            }

        } else if (nBytes > 0) {
            // throw away these bytes and hope that next time we get a solid read
            LOG_DEBUG("notifyHandler: Bad read of %ld bytes, wanted %d\n", nBytes, kSize);
            ioThread->breakLoop(true);
            return;
        } else if (nBytes == 0) {
            LOG_DEBUG("notifyHandler: Notify socket closed!\n");
            ioThread->breakLoop(false);
            // exit the loop
            break;
        } else { // nBytes < 0

            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                LOG_DEBUG("TNonblocking: notifyHandler read() failed: [%d]\n",EVUTIL_SOCKET_ERROR());
                ioThread->breakLoop(true);
                return;
            }
            // exit the loop

            break;
        }
    } while (true);
}

bool IOThread::notify(TConnection* conn) {
    evutil_socket_t fd = getNotificationSendFD();
    if (fd < 0) {
        return false;
    }

    fd_set wfds, efds;
    long ret = -1;
    long kSize = sizeof(conn);
    const char* pos = reinterpret_cast<const char*>(&conn);

    while (kSize > 0) {

        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        FD_SET(fd, &wfds);
        FD_SET(fd, &efds);
        ret = select(static_cast<int>(fd + 1), NULL, &wfds, &efds, NULL);
        if (ret < 0) {
            return false;
        } else if (ret == 0) {
            continue;
        }

        if (FD_ISSET(fd, &efds)) {
            EVUTIL_CLOSESOCKET(fd);
            return false;
        }

        if (FD_ISSET(fd, &wfds)) {
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
