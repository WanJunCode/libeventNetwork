// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

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


// void HttpServer::onConnection(const TcpConnectionPtr& conn)
// {
//   if (conn->connected())
//   {
//     // 设置　conn 的万能内容为　HttpContext
//     conn->setContext(HttpContext());
//   }
// }

// // 回调入口
// void HttpServer::onMessage(const TcpConnectionPtr& conn,
//                            Buffer* buf,
//                            Timestamp receiveTime)
// {
//   // 获得　conn 的　HttpContext
//   HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

//   // 解析　http 请求
//   if (!context->parseRequest(buf, receiveTime))
//   {
//     // 解析失败直接　404
//     conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
//     // 关闭连接
//     conn->shutdown();
//   }

//   if (context->gotAll())// 判断是否已经获得全部的信息
//   {
//     // 相应　请求　req
//     onRequest(conn, context->request());
//     context->reset();
//   }
// }

// void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
// { 
//   // 获得　http 头部的　Connection 值
//   const string& connection = req.getHeader("Connection");
//   // 判断是否关闭
//   bool close = connection == "close" ||
//     (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

//   HttpResponse response(close);
//   // 回调函数　根据　req 填充　response
//   httpCallback_(req, &response);
//   Buffer buf;
//   // 将回复添加到 buffer 中
//   response.appendToBuffer(&buf);
//   // 发送　buffer
//   conn->send(&buf);

//   // 判断是否是长连接
//   if (response.closeConnection())
//   {
//     conn->shutdown();
//   }
// }

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
