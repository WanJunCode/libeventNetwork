#include "TConnection.h"
#include "log.h"
#include <string.h>
#include "Package/ChatPackage.h"

typedef unsigned char BYTE;

TConnection::TConnection(TSocket *sock, MainServer *server)
    : socket_(sock),
    server_(server),
    base_(server_->getBase()),
    appstate(AppState::APP_CLOSE_CONNECTION){
    LOG_DEBUG("TConnection structure ... socket [%d]\n", socket_->getSocketFD());
    init();
}

TConnection::TConnection(TSocket *sock, IOThread *iothread)
    : socket_(sock),
    iothread_(iothread),
    base_(iothread_->getBase()){
    server_ = iothread_->getServer();
    if(server_ == NULL){
        LOG_DEBUG("iothread get server but is null\n");
    }
    LOG_DEBUG("TConnection structure iothread ... socket [%d]\n", socket_->getSocketFD());
    init();
}

TConnection::~TConnection(){
    LOG_DEBUG("析构函数 TConnection ... \n");
    if(socket_ != NULL){
        delete socket_;
        socket_ =NULL;
    }
    if(bev != NULL){
        bufferevent_free(bev);
        bev = NULL;
    }
}

// 初始化设置，重新设置 TSocket
void 
TConnection::init(){
    appstate = AppState::TRANS_INIT;
    socketState = SocketState::SOCKET_RECV_FRAMING;

    lastUpdate_=time(NULL);
    maxBufferSize_ = server_->getBufferSize();

    LOG_DEBUG("TConnection init bufferevent...\n");
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

        readWant_ = 0;
        frameSize_ = 0;
    } else {
        LOG_DEBUG("TConnection create bufferevent fail ...\n");
    }
}

void 
TConnection::setSocket(TSocket *socket)
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
void 
TConnection::transition()
{
    static int deep = 0;
    deep++;
    // LOG_DEBUG("into transition !!!!!!!!!!!!!!!!!!  deep [%d]\n",deep);
    switch(appstate){
        case AppState::TRANS_INIT:
            // LOG_DEBUG("trans init\n");
            appstate = AppState::APP_INIT;
            bufferevent_enable(bev,EV_READ | EV_WRITE | EV_PERSIST);
            transition();
            break;
        case AppState::APP_INIT:
            // LOG_DEBUG("app init\n");
            socketState = SocketState::SOCKET_RECV_FRAMING;
            appstate = AppState::APP_READ_FRAME_SIZE;
            bufferevent_setwatermark(bev, EV_READ, 0, maxBufferSize_);
            // Work the socket right away
            workSocket();
            break;
        case AppState::APP_READ_FRAME_SIZE:
            // LOG_DEBUG("app read frame size\n");
            // 已经读取到了一个完整的数据包的大小
            // Move into read request state
            if (0==readWant_) {
                //　已经读取到完整的数据包
                appstate = AppState::APP_READ_REQUEST;
                transition();
            } else {
               // 还有需要读取的数据　readWant != 0
                socketState = SocketState::SOCKET_RECV; //　下一次读取
                appstate = AppState::APP_READ_REQUEST;
                // 设置剩余要读取的数据包长度　为　readWant_
                bufferevent_setwatermark(bev, EV_READ, readWant_, maxBufferSize_);
            }
            break;

        case AppState::APP_READ_REQUEST:
            LOG_DEBUG("app read request\n");
            read_request();
            break;
        default:
            LOG_DEBUG("Unexpect application state!");
            return;
    }
    // LOG_DEBUG("into transition !!!!!!!!!!!!!!!!!!  deep [%d]\n",deep);
    deep--;
    // LOG_DEBUG("TConnection transport ...离开\n");
}

void 
TConnection::read_request(){
    LOG_DEBUG("读取到了一个完整的数据包 开始处理数据包\n");
    // 读取到了一个完整的数据包　｜｜　第二次读取到数据包剩余数据时
    // We are done reading the request, package the read buffer into transport
    // and get back some data from the dispatch function
    // server_->incrementActiveProcessors();

    //其实就是取出bufferevent中的output
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer_iovec image;
    if (evbuffer_peek(input, -1, NULL, &image, 1)) {
        BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);
        Package *pkg = server_->getProtocol()->getOnePackage(tmp_ptr, frameSize_);
        if (!pkg) {
            LOG_DEBUG("construct TPackage failure 数据包构造失败\n");
        } else {
            // 是否是线程池处理
            // ChatPackage *cpkg = dynamic_cast<ChatPackage *>(pkg);
            // record((ChatPackage *)(pkg));

            pkg->debug();
            auto m = pkg->innerData();            
            auto length = send(socket_->getSocketFD(), m.c_str() , m.length() ,0);
            LOG_DEBUG("buffer [%s] strlenbuffer [%d] length = [%d]\n",m.data(), m.length() ,length);

#ifdef GRPC
            server_->grpcMethod(pkg->innerData());
#endif
            delete pkg;
        }
    }
    // LOG_DEBUG("after construct one package framesize [%d]\n",frameSize_);
    evbuffer_drain(input, frameSize_);
    // The application is now on the task to finish
    appstate = AppState::APP_INIT;
    transition();
}

MainServer *
TConnection::getServer()
{
    return server_;
}

TSocket *
TConnection::getSocket()
{
    return socket_;
}

void 
TConnection::close()
{
    appstate=AppState::APP_CLOSE_CONNECTION;
    // if(socket_)
    //     socket_->close();

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

bool 
TConnection::notify(){
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

    LOG_DEBUG("TConnection error callback so close connection\n");
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
            // LOG_DEBUG("SocketState::SOCKET_RECV_FRAMING   开始接受数据\n");
            recv_framing();
            break;
        case SocketState::SOCKET_RECV:
            transition();
            break;
        default:
            break;
    }
    LOG_DEBUG("Work Socket end\n");
}

void
TConnection::recv_framing(){
    struct evbuffer *input = bufferevent_get_input(bev);
    // LOG_DEBUG("before read input length = %lu\n", evbuffer_get_length(input));
    struct evbuffer_iovec image;
    int ret = evbuffer_peek(input, -1, NULL, &image, 1);
    if (ret){
        BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);
        size_t framePos = 0;

        LOG_DEBUG("Recv RawData : [%s]\n", byteTohex((void *)tmp_ptr, image.iov_len).c_str());

        if(server_->getProtocol()->parseOnePackage(tmp_ptr,image.iov_len,framePos,frameSize_,readWant_)){
            LOG_DEBUG("framepos [%d]  framesize [%d]  readwant [%d]\n",framePos,frameSize_,readWant_);
            LOG_DEBUG("parse one package true\n");
            if(framePos > 0){
                LOG_DEBUG("frame position [%d] greater than zero\n",framePos);
                evbuffer_drain(input,framePos);
            }
            // 接收到一个完整的数据包，开始处理数据包
            transition();
        }else if(framePos > 0){
            LOG_DEBUG("Can't parse one package true\n");
            // 没收接收到有用的数据包，则丢弃多余的数据
            evbuffer_drain(input,framePos);
        }

        // std::shared_ptr<ChatPackage> pkg = make_shared<ChatPackage>(ChatPackage::CRYPT_UNKNOW,ChatPackage::DATA_STRING,image.iov_base,image.iov_len);
        // record(pkg.get());
    } else {
        LOG_DEBUG("evbuffer_peek 失败\n");
    }
    // LOG_DEBUG("recv framing end\n");
}