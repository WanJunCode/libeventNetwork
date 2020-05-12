
#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include "../base/copyable.h"

#include "httpRequest.h"


class Buffer;

class HttpContext : public copyable
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,       // 等待请求行
        kExpectHeaders,           // 等待报文头
        kExpectBody,              // 等待函数体
        kGotAll,                  // 获得全部请求
    };

    // 初始装填等待请求行
    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    // 默认的复制函数，析构函数，赋值函数
    // default copy-ctor, dtor and assignment are fine

    // return false if any error
    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const
    { return state_ == kGotAll; }

    void reset(){
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const
    { return request_; }

    HttpRequest& request()
    { return request_; }

private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};

#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H