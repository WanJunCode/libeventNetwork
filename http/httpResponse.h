
#ifndef MUDUO_NET_HTTP_HTTPRESPONSE_H
#define MUDUO_NET_HTTP_HTTPRESPONSE_H

#include "../base/copyable.h"
#include "../base/Types.h"

#include <map>

class Buffer;
class HttpResponse : public copyable
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    // close 表示是否是长连接
    explicit HttpResponse(bool close)
        : statusCode_(kUnknown),
        closeConnection_(close)
    {
    }

    // 设置状态值
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    // 设置状态信息
    void setStatusMessage(const string& message)
    { statusMessage_ = message; }

    // 设置是否短连接，发送完后即关闭
    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    // 设置　内容类型
    void setContentType(const string& contentType)
    { addHeader("Content-Type", contentType); }

    // FIXME: replace string with StringPiece　添加头部
    void addHeader(const string& key, const string& value)
    { headers_[key] = value; }

    // 设置　主体内容
    void setBody(const string& body)
    { body_ = body; }

    // 序列化到缓冲区中
    void appendToBuffer(Buffer* output) const;

    private:
    std::map<string, string> headers_;      // 数据包头
    HttpStatusCode statusCode_;             // http 状态码
    // FIXME: add http version
    string statusMessage_;                  // 状态信息字符串
    bool closeConnection_;                  // 是否关闭连接
    string body_;                           // http报文　主体
};

#endif  // MUDUO_NET_HTTP_HTTPRESPONSE_H
