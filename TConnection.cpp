#include "TConnection.h"

typedef unsigned char BYTE;

TConnection::TConnection(TSocket *sock, MainServer *server)
    : socket_(sock),
    server_(server),
    base_(server_->getBase())
{
    printf("TConnection structure ... socket [%d]\n", socket_->getSocketFD());

    init();
}

TConnection::~TConnection(){
    printf("析构函数 TConnection ... \n");
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
    printf("TConnection init ...\n");
    bev = bufferevent_socket_new(base_, socket_->getSocketFD(),BEV_OPT_CLOSE_ON_FREE);
    if (bev)
    {
        bufferevent_setcb(bev, read_cb, NULL, error_cb, this);
        bufferevent_disable(bev,EV_READ | EV_WRITE);
    }
    else
    {
        printf("TConnection create bufferevent fail ...\n");
    }
}

void TConnection::setSocket(TSocket *socket)
{
    if(socket)
    {
        printf("TConnection 重新设置 socket \n");
        socket_ = socket;
        init();
    }
    else
    {
        socket_ = NULL;
        printf("TConnection 设置 socket 为空 \n");
    }
}


void TConnection::transition()
{
    printf("TConnection transport ...\n");
    bufferevent_enable(bev,EV_READ | EV_WRITE | EV_PERSIST);
}

void TConnection::read_cb(struct bufferevent *bev, void *args)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    printf("\n\nbefore read input length = %lu\n", evbuffer_get_length(input));
    struct evbuffer_iovec image;

    int ret = evbuffer_peek(input, -1, NULL, &image, 1);
    printf("evbuffer_peek = [%d]\n", ret);
    if (ret)
    {
        // BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);
        std::string msg;
        msg.append((char *)image.iov_base, image.iov_len);
        printf("socket: [%d] from evbuffer %s \n", bufferevent_getfd(bev), msg.c_str());
    }

    evbuffer_drain(input, image.iov_len);
    printf("after read input length = %lu\n", evbuffer_get_length(input));
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
    if(socket_)
    {
        socket_->close();
    }

    if(bev != NULL)
    {
        bufferevent_free(bev);
    }
    bev=NULL;

    server_->returnTConnection(this);
}

void TConnection::error_cb(struct bufferevent *bev, short what, void *args)
{
    TConnection *conn = (TConnection *)args;

    if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF))
    {
        printf("client bufferevent eof...\n");
    }

    conn->close();
}