#include "TConnectionDispatcher.h"
#include "logcpp.h"

TConnectionDispatcher::~TConnectionDispatcher(){
    LOG_DEBUG("TConnectionDispatcher start dtor...\n");
    LOG_DEBUG("%s\n",debug().c_str());

    // active socket all in active TConnection
    while(!reuseSocketQueue.empty()){
        auto tmp = reuseSocketQueue.front();
        reuseSocketQueue.pop();
        delete tmp;
    }

    for(auto iter: activeTConnectionVector){
        ReuseConnectionQueue.push(iter.second);
    }

    while(!ReuseConnectionQueue.empty()){
        auto tmp = ReuseConnectionQueue.front();
        ReuseConnectionQueue.pop();
        delete tmp;
    }
    LOG_DEBUG("TConnectionDispatcher end dtor...\n");
    LOG_DEBUG("%s\n",debug().c_str());
}

TSocket *TConnectionDispatcher::setSocket(evutil_socket_t client){
    // ! client fd 设置为非阻塞
    evutil_make_socket_nonblocking(client);
    TSocket *sock = NULL;
    // RAII 最佳实践 (保护资源)
    std::lock_guard<std::mutex> locker(sockMutex_);
    // 复用一个 TSocket 完成对 client fd 的封装
    if (reuseSocketQueue.empty()){
        // 没有可复用的则创建新的 TSocket
        sock = new TSocket(client);
    }else{
        // 在此处将最前端的TSocket复用，后面会 pop 弹出
        sock = reuseSocketQueue.front();
        sock->setSocketFD(client);
        reuseSocketQueue.pop();
    }
    activeSocket.insert(std::pair<evutil_socket_t, TSocket *>(client, sock));
    return sock;
}

void TConnectionDispatcher::setConnection(TSocket *sock){
    TConnection *conn;
    {
        std::lock_guard<std::mutex> locker(connMutex_);
        if (ReuseConnectionQueue.empty()){
            // 选择 iothread 创建 TConnection
            conn = new TConnection(sock, mainServer->getRandomIOThread());
        }else{
            conn = ReuseConnectionQueue.front();
            ReuseConnectionQueue.pop();
            conn->setSocket(sock);
        }

        if(conn){
            // 使用管道将指针发送给IOThread开启 connection 的 bufferevent
            conn->notify();
            // 所有的TConnection 由MainServer管理，但是分派到不同的线程中进行数据读取与发送
            activeTConnectionVector.insert(std::pair<evutil_socket_t, TConnection *>(conn->getFd(),conn));
        }else{
            LOG_ERROR("socket --> TConnection 失败 ...\n");
        }
    }
}

void TConnectionDispatcher::handleConn(evutil_socket_t client){
    TSocket *sock = setSocket(client);
    if(sock){
        setConnection(sock);
    }else{
        LOG_ERROR("new client set Socket fail...\n");
    }
}

void TConnectionDispatcher::returnTConnection(TConnection *conn){
    TSocket *sock = conn->getSocket();
    if(sock==nullptr){
        LOG_ERROR("sock didn't exists\n");
        return;
    }
    LOG_DEBUG("TConnectionDispatcher return TConnection...\n");
    {
        // 从 active map 中删除
        std::lock_guard<std::mutex> locker(sockMutex_);
        auto iter = activeSocket.find(sock->getSocketFD());
        if (iter != activeSocket.end()){
            LOG_DEBUG("active socket map erase sock [%d]\n",iter->first);
            activeSocket.erase(iter);
        }else{
            LOG_ERROR("activeSocket can't find socket[%d]...\n",sock->getSocketFD());
        }
        sock->close();
        // 关闭回收的TSocket连接
        reuseSocketQueue.push(sock);
    }

    {
        std::lock_guard<std::mutex> locker(connMutex_);
        LOG_DEBUG("TConnectionDispatcher 回收 TConnection start ...\n");
        // 获得 TConnection 包装的 socket 回收 sock
        auto iter = activeTConnectionVector.find(conn->getFd());
        if (iter != activeTConnectionVector.end()){
            LOG_DEBUG("active connection map erase connection [%d]\n",iter->first);
            activeTConnectionVector.erase(iter);
        }else{
            LOG_ERROR("active connection can't find socket[%d]...\n",conn->getFd());
        }

        // ReuseConnectionQueue 中 conn 都带有一个关闭的 TSocket
        // 断开 TConnection 与 TSocket 之间的关系
        conn->setSocket(NULL);
        ReuseConnectionQueue.push(conn);
    }
}

bool TConnectionDispatcher::isActive(TConnection *conn){
    auto iter = activeTConnectionVector.find(conn->getFd());
    if(iter == activeTConnectionVector.end()){
        return false;
    }else{
        return true;
    }
}

void TConnectionDispatcher::heartBeat(){
    for(auto iter : activeTConnectionVector){
        iter.second->heartBeat();
    }
}

std::string TConnectionDispatcher::debug(){
    std::ostringstream oss;
    oss<<"from :Cloud Server"<<endl;
    oss<<"active TConnection size ="<<activeTConnectionVector.size()<<endl;
    oss<<"reuseable TConnection size ="<<ReuseConnectionQueue.size()<<endl;
    oss<<"active TSocket size = "<<activeSocket.size()<<endl;
    oss<<"reuseable TSocket size = "<<reuseSocketQueue.size()<<endl;
    return oss.str();
}