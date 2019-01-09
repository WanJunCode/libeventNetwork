#include "TConnection.h"

#include "log.h"

typedef unsigned char BYTE;

TConnection::TConnection(TSocket *sock, MainServer *server)
    : socket_(sock),
    server_(server),
    base_(server_->getBase())
{
    LOG_DEBUG("TConnection structure ... socket [%d]\n", socket_->getSocketFD());

    init();
}

TConnection::TConnection(TSocket *sock, IOThread *iothread)
    : socket_(sock),
    iothread_(iothread),
    base_(iothread_->getBase())
{
    server_ =NULL;
    server_ = iothread_->getServer();
    if(server_==NULL){
        LOG_DEBUG("iothread get server but is null\n");
    }

    LOG_DEBUG("TConnection structure iothread ... socket [%d]\n", socket_->getSocketFD());

    init();
}

TConnection::~TConnection()
{
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
void TConnection::init()
{
    LOG_DEBUG("TConnection init bufferevent...\n");
    bev = bufferevent_socket_new(base_, socket_->getSocketFD(),BEV_OPT_CLOSE_ON_FREE);
    if (bev) {
        bufferevent_setcb(bev, read_cb, NULL, error_cb, this);
        // 初始化后暂停 bufferevent的使用
        bufferevent_disable(bev,EV_READ | EV_WRITE);
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
    else
    {
        LOG_DEBUG("============================================\n");
        LOG_DEBUG("TConnection close but server is null!!!\n");
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
    struct evbuffer *input = bufferevent_get_input(bev);
    LOG_DEBUG("before read input length = %lu\n", evbuffer_get_length(input));
    struct evbuffer_iovec image;

    int ret = evbuffer_peek(input, -1, NULL, &image, 1);
    LOG_DEBUG("evbuffer_peek return block size= [%d]\n", ret);
    if (ret)
    {
        // BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);
        std::string msg((char *)image.iov_base, image.iov_len);
        LOG_DEBUG("socket: [%d] from evbuffer %s \n",bufferevent_getfd(bev), msg.c_str());
    }

    evbuffer_drain(input, image.iov_len);
    LOG_DEBUG("after read input length = %lu\n\n", evbuffer_get_length(input));
}
