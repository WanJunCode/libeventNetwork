#include "TConnection.h"
#include "log.h"

#include <string.h>
#include "Package/ChatPackage.h"

typedef unsigned char BYTE;

TConnection::TConnection(TSocket *sock, MainServer *server)
    : socket_(sock),
    server_(server),
    base_(server_->getBase()),
    appstate(AppState::APP_CLOSE_CONNECTION)
{
    LOG_DEBUG("TConnection structure ... socket [%d]\n", socket_->getSocketFD());

    init();
}

TConnection::TConnection(TSocket *sock, IOThread *iothread)
    : socket_(sock),
    iothread_(iothread),
    base_(iothread_->getBase())
{
    server_ = iothread_->getServer();
    if(server_ == NULL){
        LOG_DEBUG("iothread get server but is null\n");
    }

    LOG_DEBUG("TConnection structure iothread ... socket [%d]\n", socket_->getSocketFD());

    init();
}

TConnection::~TConnection(){

    LOG_DEBUG("析构函数 TConnection ... \n");
    if(socket_ != NULL)
    {
        delete socket_;
        socket_ =NULL;
    }

    if(bev != NULL)
    {
        bufferevent_free(bev);
        bev = NULL;
    }
}

// 初始化设置，重新设置 TSocket
void TConnection::init(){
    appstate=AppState::TRANS_INIT;
    socketState = SocketState::SOCKET_RECV_FRAMING;

    lastUpdate_=time(NULL);

    LOG_DEBUG("TConnection init bufferevent...\n");
    int maxBufferSize_ = server_->getBufferSize();
    bev = bufferevent_socket_new(base_, socket_->getSocketFD(),BEV_OPT_CLOSE_ON_FREE);
    if (bev) {
        if(socket_->getSocketFD() == INVALID_SOCKET){
            // FD 无效
            struct sockaddr_in addr;
            memset(&addr,0,sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(socket_->getLocalHost().c_str());
            addr.sin_port = htons(socket_->getLocalPort());
            bzero(&(addr.sin_zero),8);
            
            if( -1 == bufferevent_socket_connect(bev,(sockaddr*)&addr,sizeof(addr)) ){
                LOG_DEBUG("TConnection fd [%d] bufferevent_socket_connect fail!\n",socket_->getSocketFD());
                close();
            }else{
                LOG_DEBUG("TConnection fd [%d] init success!\n",socket_->getSocketFD());
            }
        }

        bufferevent_setcb(  bev, 
                            TConnection::read_cb, 
                            NULL, 
                            TConnection::error_cb, 
                            this);
        bufferevent_setwatermark(bev, EV_READ | EV_WRITE, 0, maxBufferSize_);
        // 初始化后暂停 bufferevent的使用
        bufferevent_disable(bev,EV_READ | EV_WRITE);
        server_->getRedis()->executeCommand("lpush user %s",socket_->getSocketInfo().c_str());
    } else {
        LOG_DEBUG("TConnection create bufferevent fail ...\n");
    }
}

void TConnection::setSocket(TSocket *socket)
{
    if(socket) {
        LOG_DEBUG("TConnection 重新设置 socket \n");
        socket_ = socket;
        init();
    } else {
        socket_ = NULL;
        LOG_DEBUG("TConnection 设置 socket 为空 \n");
    }
}

// 开启 client 在 base 上的 bufferevent
void TConnection::transition()
{
    LOG_DEBUG("TConnection transport ...\n");
    bufferevent_enable(bev,EV_READ | EV_WRITE | EV_PERSIST);
}

MainServer *TConnection::getServer()
{
    return server_;
}

TSocket *TConnection::getSocket()
{
    return socket_;
}

void TConnection::close()
{
    appstate=AppState::APP_CLOSE_CONNECTION;
    // if(socket_)
    // {
    //     socket_->close();
    // }

    if(bev != NULL)
    {
        // 释放 TSocket 上建立的 bufferevent
        bufferevent_free(bev);
        bev=NULL;
    }

    // 还是需要 MainServer 去处理 TConnection
    if(server_!=NULL)
    {
        LOG_DEBUG("TConnection close return tconnection\n");
        server_->returnTConnection(this);
    }
}

bool TConnection::notify(){
    if(iothread_){
        return iothread_->notify(this);
    }
    return false;
}

// static
void TConnection::error_cb(struct bufferevent *bev, short what, void *args)
{
    // client disconnection
    TConnection *conn = (TConnection *)args;

    if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF))
    {
        LOG_DEBUG("client bufferevent eof...\n");
    }

    // client 断开连接
    conn->close();
}

// static
void TConnection::read_cb(struct bufferevent *bev, void *args)
{
    TConnection *conn = (TConnection*)args;
    conn->lastUpdate_ = time(NULL);
    conn->workSocket();

}

void
TConnection::record(ChatPackage *pkg){
    std::string message;
    message.append((char*)pkg->data(),pkg->length());
    server_->getRedis()->executeCommand("lpush %s %s",socket_->getPeerHost().c_str(),message.c_str());
}

void
TConnection::workSocket(){
    switch(socketState){
        case SocketState::SOCKET_RECV_FRAMING:
            recv_framing();
            break;
        case SocketState::SOCKET_RECV:
            recv();
            break;
        default:
            break;
    }
}

void
TConnection::recv_framing(){
    struct evbuffer *input = bufferevent_get_input(bev);
    LOG_DEBUG("before read input length = %lu\n", evbuffer_get_length(input));
    struct evbuffer_iovec image;
    int ret = evbuffer_peek(input, -1, NULL, &image, 1);
    if (ret){
        // void * =>  unsigned char *
        BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);

        std::string msg((char *)image.iov_base, image.iov_len);
        LOG_DEBUG("socket: [%d] from evbuffer [%s] \n",bufferevent_getfd(bev), msg.c_str());
        LOG_DEBUG("Recv RawData:[%s]\n", byteTohex((void *)tmp_ptr, image.iov_len).c_str());
        size_t framePos = 0;

        if(server_->getProtocol()->parseOnePackage(tmp_ptr,image.iov_len,framePos,frameSize_,readWant_)){
            LOG_DEBUG("parse one package true\n");
        }else{
            LOG_DEBUG("parse one package false\n");
        }

        // 在此处将接受的到数据打包成一个　package
        std::shared_ptr<ChatPackage> pkg = make_shared<ChatPackage>(ChatPackage::CRYPT_UNKNOW,ChatPackage::DATA_STRING,image.iov_base,image.iov_len);
        record(pkg.get());
    }
    // 从　input 缓冲区中丢弃　　image.iov_len 长度的数据
    evbuffer_drain(input, image.iov_len);
    LOG_DEBUG("after read input length = %lu\n\n", evbuffer_get_length(input));
}

void
TConnection::recv(){

}