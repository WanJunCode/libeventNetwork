
#include "httpServer.h"

#include "httpContext.h"
#include "httpRequest.h"
#include "httpResponse.h"

#include "../base/Buffer.h"
#include "../main/logcpp.h"
#include "../main/TConnection.h"

// 默认的　http 回调函数　404
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp){
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "Muduo");
    resp->setBody("hello, world!\n");
}

HttpServer::HttpServer()
    :httpCallback_(defaultHttpCallback)    // 设置默认回调函数
{
    LOG_DEBUG("http server init\n");
}

HttpServer::~HttpServer(){
    LOG_DEBUG("http server dtor\n");
}

void HttpServer::response(TConnection *conn){
    HttpResponse resp(false);

    resp.setStatusCode(HttpResponse::k200Ok);
    resp.setStatusMessage("OK");
    resp.setContentType("text/plain");
    resp.addHeader("Server", "Muduo");
    resp.setBody("hello, world!\n");

    Buffer buf;
    // 将回复添加到 buffer 中
    resp.appendToBuffer(&buf);
    conn->write(buf);
}

bool HttpServer::onMessage(TConnection *conn, Buffer& buf){
    HttpContext context; // stack variable
    auto timestamp = Timestamp::now();
    // context 从　buf　获得　request
    if(!context.parseRequest(&buf,timestamp)){
        return false;
    }

    if (context.gotAll())// 判断当前的状态是否已经获得全部请求数据
    {
        onRequest(conn, context.request());
        context.reset();
    }

    return true;
}

void HttpServer::onRequest(TConnection *conn, HttpRequest &request){
    const string& connection = request.getHeader("Connection");
    bool close = connection == "close" ||
        (request.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

    HttpResponse response(close);
    // 回调函数　根据　req 填充　response
    httpCallback_(request, &response);
    Buffer buf;
    // 将回复添加到 buffer 中
    response.appendToBuffer(&buf);
    // 发送　buffer
    conn->write(buf);

    // 判断是否是长连接
    if (response.closeConnection())
    {
        LOG_DEBUG("http close this connection\n");
    }
}
