#include "TSocket.h"
#include <stdio.h>
#include <sstream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <string.h>
#include <type_traits>

#include "logcpp.h"

template <class DesType, class SrcType>
DesType convert(const SrcType &t)
{
    std::stringstream stream;
    stream << t;
    DesType result;
    stream >> result;
    return result;
}

#ifndef SOCKOPT_CAST_T
#ifndef _WIN32
#define SOCKOPT_CAST_T void     // linux
#else
#define SOCKOPT_CAST_T char     // win32
#endif // _WIN32
#endif


// 转换成 const sockopt
// 将 T 类型转化为 const SOCKOPT_CAST_T 类型
template <class T>
inline const SOCKOPT_CAST_T *const_cast_sockopt(const T *v)
{
    return reinterpret_cast<const SOCKOPT_CAST_T *>(v);
}

// 转换成 sockopt
template <class T>
inline SOCKOPT_CAST_T *cast_sockopt(T *v)
{
    return reinterpret_cast<SOCKOPT_CAST_T *>(v);
}

/**
 * TSocket implementation. 实现
 */
TSocket::TSocket()
    : socket_(INVALID_SOCKET)
{
}

TSocket::TSocket(evutil_socket_t sock)
    : socket_(sock)
{
    getSocketInfo();
#ifdef SO_NOSIGPIPE
    {
        int one = 1;
        // 1 打开或关闭调试信息
        //   int socket, int level, int option_name,const void *option_value, size_t option_len);
        setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
        // 目的是不接收由于Client异常断开造成的 Broken pipe 信号
    }
#endif
}

TSocket::TSocket(const std::string &host, int port)
    : socket_(INVALID_SOCKET)
{
    
    // 赋值本地 host port
    socket_info_.localHost_ = host;
    socket_info_.localPort_ = port;
}

TSocket::~TSocket()
{
    close();
    LOG_DEBUG("TSocket 析构函数\n");
}

std::string getpeermac(int sockfd)
{
    // 定义接口的最大值
#define MAXINTERFACES 16
    // Interface request structure
    struct ifreq buf[MAXINTERFACES] = {};
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;

    // 获得 socket conf 信息    sio get if config
    if (!ioctl(sockfd, SIOCGIFCONF, (char *)&ifc))
    {
        int intrface = ifc.ifc_len / sizeof(struct ifreq);

        while (intrface-- > 0)
        {
            struct arpreq arpreq;
            struct sockaddr_in dstadd_in;
            socklen_t len = sizeof(struct sockaddr_in);
            memset(&arpreq, 0, sizeof(struct arpreq));
            memset(&dstadd_in, 0, sizeof(struct sockaddr_in));

            if (getpeername(sockfd, (struct sockaddr *)&dstadd_in, &len) < 0)
            {
                // 执行失败
                LOG_DEBUG("getpeername()");
            }
            else
            {
                // 网络协议   dstadd_in -->  arpreq
                memcpy(&arpreq.arp_pa, &dstadd_in, sizeof(struct sockaddr_in));
                strcpy(arpreq.arp_dev, buf[intrface].ifr_name);
                arpreq.arp_pa.sa_family = AF_INET;
                arpreq.arp_ha.sa_family = AF_UNSPEC;

                // 获得arp            get arp
                if (::ioctl(sockfd, SIOCGARP, &arpreq) < 0)
                {
                    // 执行失败
                    LOG_DEBUG("ioctl() SIOCGARP");
                }
                else
                {
                    char macAddr[64] = {0x00};
                    unsigned char *ptr = (unsigned char *)arpreq.arp_ha.sa_data;
                    sprintf(macAddr, "%02x%02x%02x%02x%02x%02x", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
                    return macAddr;
                }
            }
        }
    }
    return "";
}

// 解析 socket_
void TSocket::parseSocket()
{
    // 作用 类似于 sockaddr 不过有更大空间存储 ipv4/ipv6，用于 getpeername 的信息存储
    struct sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    // 连接之后 调用底层函数  获得 peer.name 存入 sockaddr_storage  addr
    if (0 == ::getpeername(socket_, (sockaddr *)&addr, &addrLen))
    {
        // sockaddr_storage* --> sockaddr*
        struct sockaddr *addrPtr = (sockaddr *)&addr;

        char clienthost[NI_MAXHOST] = {0x00};
        char clientservice[NI_MAXSERV] = {0x00};

        // 它以一个套接口地址为参数，返回一个描述主机的字符串和一个描述服务的字符串
        getnameinfo(addrPtr,    // 套接口地址
                    addrLen,    // 套接口长度
                    clienthost, // 存储对等端 host
                    sizeof(clienthost),
                    clientservice, // 存储对等端 port
                    sizeof(clientservice),
                    NI_NUMERICHOST | NI_NUMERICSERV);

        // 存储对等端 的 host 和 port
        socket_info_.peerHost_ = clienthost;
        socket_info_.peerPort_ = convert<int>(clientservice);
        //socket_info_.peerMac_ = getpeermac(socket_);
    }

    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);
    // 获得本地 socket 的信息   未连接的 socket 使用 getsockname 获得内核分配的 host 、port
    if (0 == ::getsockname(socket_, (struct sockaddr *)&sa, &salen))
    {
        if (sa.ss_family == AF_INET)
        {
            // 如果协议是  AF_INET
            // sockaddr_storage 转化 sockaddr_in
            struct sockaddr_in *s = (struct sockaddr_in *)&sa;
            // 存储本地 host  port
            socket_info_.localHost_ = ::inet_ntoa(s->sin_addr);
            socket_info_.localPort_ = ::ntohs(s->sin_port);
        }
    }
}

bool TSocket::isOpen()
{
    return (INVALID_SOCKET != socket_);
}

void TSocket::close()
{
    if (INVALID_SOCKET != socket_)
    {
        // 使用兼容的 socket 关闭方法
        EVUTIL_CLOSESOCKET(socket_);
    }
    socket_ = INVALID_SOCKET;
}

// 重新设置 内部封装的 socket_
void TSocket::setSocketFD(evutil_socket_t socket)
{
    if (INVALID_SOCKET != socket_)
    {
        close();
    }
    socket_ = socket;
    parseSocket(); // 对新的 socket_ 进行解析

#ifdef SO_NOSIGPIPE
    {
        int one = 1;
        // 忽略 sigpipe 信号
        setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
    }
#else
    {
        int one = 1;
        setsockopt(socket_, SOL_SOCKET, MSG_NOSIGNAL, &one, sizeof(one));
    }
#endif
    setKeepAlive(true);
}

void TSocket::setLinger(bool on, int linger)
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    socket_setting_.lingerOn_ = on;
    socket_setting_.lingerVal_ = linger;

#ifndef _WIN32
    struct linger l = {(socket_setting_.lingerOn_ ? 1 : 0),
                       socket_setting_.lingerVal_};
#else
    struct linger l = {static_cast<u_short>(socket_info_.lingerOn_ ? 1 : 0),
                       static_cast<u_short>(socket_info_.lingerVal_)};
#endif

    // 设置 tcp 的 linger 属性
    if (-1 == setsockopt(socket_, SOL_SOCKET, SO_LINGER, cast_sockopt(&l), sizeof(l)))
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setLinger() setsockopt() " << getSocketInfo();
        perror("TSocket::setLinger() setsockopt() \n");
    }
}

void TSocket::setReuseAddr()
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    // Set socket to REUSEADDR
    int v = 1;
    if (-1 == setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, cast_sockopt(&v), sizeof(v)))
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setReuseAddr() setsockopt() " << getSocketInfo();
        perror("TSocket::setReuseAddr() setsockopt() \n");
    }
}

void TSocket::setNoDelay(bool noDelay)
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    socket_setting_.noDelay_ = noDelay;
    // Set socket to NODELAY
    int v = socket_setting_.noDelay_ ? 1 : 0;
    int ret = setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, cast_sockopt(&v), sizeof(v));
    if (ret == -1)
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setNoDelay() setsockopt() " << getSocketInfo();
        perror("TSocket::setNoDelay() setsockopt() \n");
    }
}

void TSocket::setConnTimeout(int ms)
{
    socket_setting_.connTimeout_ = ms;
}

// 设置 recv send 时间
void setGenericTimeout(evutil_socket_t s, int timeout_ms, int optname)
{
    if (timeout_ms < 0)
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setGenericTimeout with negative input: " << timeout_ms;
        perror("TSocket::setGenericTimeout with negative input: \n");
        return;
    }

    if (s == INVALID_SOCKET)
    {
        return;
    }

#ifdef _WIN32
    DWORD platform_time = static_cast<DWORD>(timeout_ms);
#else
    struct timeval platform_time = {(int)(timeout_ms / 1000), (int)((timeout_ms % 1000) * 1000)};
#endif

    if (-1 == setsockopt(s, SOL_SOCKET, optname, cast_sockopt(&platform_time), sizeof(platform_time)))
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setGenericTimeout() setsockopt() ";
        perror("TSocket::setGenericTimeout() setsockopt() \n");
    }
}

void TSocket::setRecvTimeout(int ms)
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    setGenericTimeout(socket_, ms, SO_RCVTIMEO);
    socket_setting_.recvTimeout_ = ms;
}

void TSocket::setSendTimeout(int ms)
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    setGenericTimeout(socket_, ms, SO_SNDTIMEO);
    socket_setting_.sendTimeout_ = ms;
}

// 设置 保持连接
void TSocket::setKeepAlive(bool keepAlive)
{
    if (INVALID_SOCKET == socket_)
    {
        return;
    }
    // 先赋值 socket设置
    socket_setting_.keepAlive_ = keepAlive;
    // bool  -->  int   用于 setsocketopt
    int value = socket_setting_.keepAlive_;

    // 底层函数 setsocketopt
    if (-1 == setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, const_cast_sockopt(&value), sizeof(value)))
    {
        // LOG_CXX(LOG_ERROR) << "TSocket::setKeepAlive() setsockopt() " << getSocketInfo();
        perror("TSocket::setKeepAlive() setsockopt() \n");
    }
}

void TSocket::setMaxRecvRetries(int maxRecvRetries)
{
    socket_setting_.maxRecvRetries_ = maxRecvRetries;
}

// 获得 对等  socket 的信息，返回值是 peer.host   peer.port
std::string TSocket::getSocketInfo()
{
    if (socket_info_.peerHost_.empty())
    {
        // 判断 对等 socket 是否存在，如果不存在调用内部函数解析 socket
        parseSocket();
    }
    std::ostringstream oss;
    // 在 parseSocket 中解析了 peerHost，peerPort
    // oss << "<Host: " << socket_info_.peerHost_ << " Port: " << socket_info_.peerPort_ << "> : "<<time(NULL);
    oss << socket_info_.peerHost_ << ":" << socket_info_.peerPort_ << ":"<<time(NULL);
    return oss.str();
}

bool TSocket::useLowMinRto_ = false;
void TSocket::setUseLowMinRto(bool useLowMinRto)
{
    useLowMinRto_ = useLowMinRto;
}
bool TSocket::getUseLowMinRto()
{
    return useLowMinRto_;
}
