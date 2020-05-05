#include "TConnection.h"
#include "logcpp.h"
#include "../Package/Adapter/AdapterMap.h"

#include <string.h>
#include <vector>
#include <stdlib.h>
typedef unsigned char BYTE;

class ProcessTask{
public:
    ProcessTask(Adapter *adapter,Package *package,TConnection *client)
    :adapter_(adapter)
    ,package_(package)
    ,client_(client){
        // LOG_DEBUG("process task 初始化\n");
    };

    ~ProcessTask(){
        LOG_DEBUG("process task 销毁\n")
        delete package_;
    }

    void run(){
        auto out = adapter_->adapter(package_);
        if(out){
            LOG_DEBUG("获得下行报文\n");
            client_->transMessage(out);
            delete out;
        }
    };

private:
    Adapter     *adapter_;
    Package     *package_;
    TConnection *client_;
};

void process(Adapter *adapter,std::unique_ptr<Package> package){

}

TConnection::TConnection(TSocket *sock, MainServer *server)
    : socket_(sock),
    server_(server),
    base_(server_->getBase()),
    appstate(AppState::APP_CLOSE_CONNECTION){
    LOG_DEBUG("TConnection structure ... socket [%d]\n", socket_->getSocketFD());
    init();
}

// 参数为 IOThread 的构造函数，从 iothread 中获得 event_base
TConnection::TConnection(TSocket *sock, IOThread *iothread)
    : socket_(sock),
    iothread_(iothread),
    base_(iothread_->getBase()){
    server_ = iothread_->getServer();
    if(server_ == NULL){
        LOG_FATAL("iothread get server but is null\n");
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
    // 默认是五秒钟心跳间隔
    heartBeat_ = 30;
    appstate = AppState::TRANS_INIT;
    socketState = SocketState::SOCKET_RECV_FRAMING;

    lastUpdate_=time(NULL);
    maxBufferSize_ = server_->getBufferSize();

    bev = bufferevent_socket_new(base_, socket_->getSocketFD(),BEV_OPT_CLOSE_ON_FREE);
    if (bev) {
        if(socket_->getSocketFD() == INVALID_SOCKET){
            LOG_ERROR("socket is invalid\n");
            // FD 无效
            struct sockaddr_in addr;
            memset(&addr,0,sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(socket_->getLocalHost().c_str());
            addr.sin_port = htons(socket_->getLocalPort());
            bzero(&(addr.sin_zero),8);
            if( -1 == bufferevent_socket_connect(bev,(sockaddr*)&addr,sizeof(addr)) ){
                LOG_WARN("TConnection fd [%d] bufferevent_socket_connect fail!\n",socket_->getSocketFD());
                close();
            }else{
                LOG_DEBUG("TConnection fd [%d] init success!\n",socket_->getSocketFD());
            }
        }

        bufferevent_setcb(  bev, TConnection::read_cb, NULL, TConnection::error_cb, this);
        // 后续的transition()会设置水位
        // bufferevent_setwatermark(bev, EV_READ | EV_WRITE, 0, maxBufferSize_);

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
TConnection::setSocket(TSocket *socket){
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
    switch(appstate){
        case AppState::TRANS_INIT:
            appstate = AppState::APP_INIT;
            bufferevent_enable(bev,EV_READ | EV_WRITE | EV_PERSIST);
            transition();
            break;
        case AppState::APP_INIT:
            socketState = SocketState::SOCKET_RECV_FRAMING;
            appstate = AppState::APP_READ_FRAME_SIZE;
            bufferevent_setwatermark(bev, EV_READ, 0, maxBufferSize_);
            // Work the socket right away
            workSocket();
            break;
        case AppState::APP_READ_FRAME_SIZE:
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
            read_request();
            break;
        case AppState::APP_CLOSE_CONNECTION:
            close();
            break;
        default:
            LOG_DEBUG("Unexpect application state!");
            return;
    }
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
        // 使用唯一指针来获得 数据包,防止数据包的多次复制
        Package *pkg = server_->getProtocol()->getOnePackage(tmp_ptr, frameSize_);
        if (!pkg) {
            LOG_ERROR("construct TPackage failure 数据包构造失败\n");
        } else {
            // 根据数据包头部信息查找适配器
            auto adapter = AdapterMap::getInstance().queryAdapter(pkg->factoryCode);
            if(adapter){
                // 如果 processtask 放入线程池中运行,不能使用栈变量,生存周期不够
                std::shared_ptr<ProcessTask> tasker(new ProcessTask(adapter.get(),pkg,this));
                server_->run(std::bind(&ProcessTask::run,tasker));
            }else{
                LOG_ERROR("factory code [%s] don't match adapters\n",pkg->factoryCode.data());
            }
        }
    }
    // 丢弃已经处理的数据
    evbuffer_drain(input, frameSize_);
    // The application is now on the task to finish
    appstate = AppState::APP_INIT;
    transition();
}

void 
TConnection::close(){
    // if(socket_)
    //     socket_->close();
    if(bev != NULL){
        // 释放 TSocket 上建立的 bufferevent
        bufferevent_free(bev);
        bev = NULL;
    }

    // 还是需要 MainServer 去处理 TConnection
    if(server_!=NULL){
        LOG_DEBUG("TConnection close return tconnection\n");
        server_->returnTConnection(this);
    }

    appstate=AppState::APP_CLOSE_CONNECTION;
}

bool 
TConnection::notify(){
    if(iothread_){
        return iothread_->notify(this);
    }
    return false;
}

void
TConnection::heartBeat(){
    appstate=AppState::APP_CLOSE_CONNECTION;
    if(heartBeat_  < (time(NULL) - lastUpdate_) ){
        // 如果不使用 notify 来关闭连接，会导致bufferevent 复用bug
        if(!notify()){
            LOG_DEBUG("fail to notify tconnection\n");
            close();    
        }
    }
}

// static
void TConnection::error_cb(struct bufferevent *bev, short what, void *args){
    UNUSED(bev);
    // client disconnection
    TConnection *conn = (TConnection *)args;

    if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF)){
        LOG_DEBUG("client bufferevent eof...\n");
    }

    LOG_DEBUG("TConnection error callback so close connection\n");
    // client 断开连接
    conn->close();
}

// static
void TConnection::read_cb(struct bufferevent *bev, void *args){
    // connrction recv new data begin here
    UNUSED(bev);
    TConnection *conn = (TConnection*)args;
    conn->lastUpdate_ = time(NULL);
    conn->workSocket();
}

void
TConnection::record(std::string message){
    server_->getRedis()->executeCommand("lpush %s %s",socket_->getPeerHost().c_str(),message.c_str());
}

void
TConnection::workSocket(){
    switch(socketState){
        case SocketState::SOCKET_RECV_FRAMING:
            recv_framing();
            break;
        case SocketState::SOCKET_RECV:
            transition();
            break;
        default:
            break;
    }
}

void
TConnection::recv_framing(){
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer_iovec image;
    int ret = evbuffer_peek(input, -1, NULL, &image, 1);
    if (ret){
        BYTE *tmp_ptr = static_cast<BYTE *>(image.iov_base);
        size_t framePos = 0;
        // 打印接受的数据
        LOG_DEBUG("Recv RawData : [%s]\n", byteTohex((void *)tmp_ptr, image.iov_len).c_str());
        if(server_->getProtocol()->parseOnePackage(tmp_ptr,image.iov_len,framePos,frameSize_,readWant_)){
            // LOG_DEBUG("framepos [%d]  framesize [%d]  readwant [%d]\n",framePos,frameSize_,readWant_);
            if(framePos > 0){
                LOG_DEBUG("frame position [%d] greater than zero\n",framePos);
                evbuffer_drain(input,framePos);
            }
            // 接收到一个完整的数据包，开始处理数据包
            transition();
        }else if(framePos > 0){
            // HTTP 请求入口
            Buffer buf;
            buf.append(image.iov_base,image.iov_len);
            if(!server_->gethttp().onMessage(this,buf)){
                LOG_WARN("Can't parse one package true\n");
            }else{
                LOG_DEBUG("http response\n");
            }
            // 没收接收到有用的数据包，则丢弃多余的数据
            evbuffer_drain(input,framePos);
        }
    }else{
        LOG_WARN("evbuffer_peek has no data inside\n");
    }
}

bool TConnection::transMessage(Package *out) {
    if (NULL != out && 0 == bufferevent_write(bev, out->getRawData(), out->getRawDataLength())) {
        LOG_DEBUG("报文下行成功\n");
        return true;
    }
    return false;
}