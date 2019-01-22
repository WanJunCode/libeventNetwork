#ifndef __CF_TSOCKET_H
#define __CF_TSOCKET_H


#include <arpa/inet.h>
#include <string>
#include <event2/util.h>

#define INVALID_SOCKET -1


// 对 evutil_socket_t 的封装，资源管理容器  析构函数中自动释放 socket 资源
class TSocket
{
public:
    explicit TSocket();
    explicit TSocket(evutil_socket_t sock);
    explicit TSocket(const std::string &host, int port);
    virtual ~TSocket();

public:
    /**
     * Whether the socket is alive.
     */
    virtual bool isOpen();

    /**
     * Shuts down communications on the socket.
     */
    virtual void close();

    /**
     * Get the host that the socket is connected to
     */
    inline std::string &getLocalHost()
    {
        return socket_info_.localHost_;
    };

    /**
     * Get the port that the socket is connected to
     */
    inline int getLocalPort()
    {
        return socket_info_.localPort_;
    };

    /**
     * Controls whether the linger option is set on the socket.
     */
    void setLinger(bool on, int linger);

    /**
     * Whether to enable/disable Nagle's algorithm.
     */
    void setNoDelay(bool noDelay);

    void setReuseAddr();

    /**
     * Set the connect timeout
     */
    void setConnTimeout(int ms);

    /**
     * Set the receive timeout
     */
    void setRecvTimeout(int ms);

    /**
     * Set the send timeout
     */
    void setSendTimeout(int ms);

    /**
     * Set the max number of recv retries in case of an THRIFT_EAGAIN error
     */
    void setMaxRecvRetries(int maxRecvRetries);

    /**
     * Set SO_KEEPALIVE
     */
    void setKeepAlive(bool keepAlive);

    /**
     * Get socket information formatted as a string <Host: x Port: x>
     * 获得 peer(对等端) host port 字符串
     */
    std::string getSocketInfo();

    /**
     * Returns the DNS name of the host to which the socket is connected
     */
    inline std::string &getPeerDomain()
    {
        return socket_info_.peerDomain_;
    };

    /**
     * Returns the address of the host to which the socket is connected
     */
    inline std::string &getPeerHost()
    {
        // 如果没有解析过 socket ，则 peer_Host 为空
        if (socket_info_.peerHost_.empty())
        {
            parseSocket();
        }
        return socket_info_.peerHost_;
    };

    /**
     * Returns the port of the host to which the socket is connected
     **/
    inline int getPeerPort()
    {
        if (socket_info_.peerHost_.empty())
        {
            parseSocket();
        }
        return socket_info_.peerPort_;
    };

    /**
    * Returns the mac of the host to which the socket is connected
    **/
    inline std::string &getPeerMac()
    {
        return socket_info_.peerMac_;
    };

    /**
     * Returns the underlying socket file descriptor.
     * 获得内部封装的 evutil_socket_t 文件描述符
     */
    inline evutil_socket_t getSocketFD() const
    {
        return socket_;
    }

    /**
     * (Re-)initialize a TSocket for the supplied descriptor.  This is only
     * intended for use by TNonblockingServer -- other use may result in
     * unfortunate surprises.
     * 这是唯一的 TNonblockingServer 接口，其他可能会导致意想不到的错误
     * TNonblockingServer 通过这个函数 设置 内部的 socket 文件描述符
     */
    void setSocketFD(evutil_socket_t fd);

    /**
     * Sets whether to use a low minimum TCP retransmission timeout.
     */
    static void setUseLowMinRto(bool useLowMinRto);

    /**
     * Gets whether to use a low minimum TCP retransmission timeout.
     */
    static bool getUseLowMinRto();

private:
    /**
    * Parse the base info from the host to which the socket is connected
    */
    void parseSocket();

protected:
    // socket 信息
    typedef struct _tag_socket_info_t
    {
        std::string localHost_;  /** Host to connect to */
        int localPort_;          /** Port number to connect on */
        std::string peerHost_;   /** Peer hostname */
        std::string peerDomain_; /** Peer domain */
        std::string peerMac_;    /** Peer MAC */
        int peerPort_;           /** Peer port */
        _tag_socket_info_t()
            : localPort_(0),
              peerPort_(0){};
    } socket_info_t;

    // socket 设置
    typedef struct _tag_socket_setting_t
    {
        int connTimeout_;    /** Connect timeout in ms */
        int sendTimeout_;    /** Send timeout in ms */
        int recvTimeout_;    /** Recv timeout in ms */
        bool keepAlive_;     /** Keep alive on */
        bool lingerOn_;      /** Linger on */
        int lingerVal_;      /** Linger val */
        bool noDelay_;       /** Nodelay */
        int maxRecvRetries_; /** Recv EGAIN retries */
        _tag_socket_setting_t() : connTimeout_(0),
                                  sendTimeout_(0),
                                  recvTimeout_(0),
                                  keepAlive_(false),
                                  lingerOn_(1), // linger 默认是 打开的
                                  lingerVal_(0),
                                  noDelay_(1),
                                  maxRecvRetries_(5){};
    } socket_setting_t;

    /** Underlying socket handle 内部的socket句柄*/
    evutil_socket_t socket_; // TSocket 封装的 socket_

    /** socket information socket 信息*/
    socket_info_t socket_info_;
    /** socket setting socket 设置*/
    socket_setting_t socket_setting_;

    // 最小tcp超时重传机制
    static bool useLowMinRto_; /** Whether to use low minimum TCP retransmission timeout */
};

#endif