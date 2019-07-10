// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include <functional>
#include "../base/noncopyable.h"

class Buffer;
class TConnection;
class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer: public noncopyable
{
 public:
  typedef std::function<void (const HttpRequest&,
                              HttpResponse*)> HttpCallback;

  HttpServer();
  ~HttpServer();

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void response(TConnection *conn);
  bool onMessage(TConnection *conn, Buffer& buf);
  void onRequest(TConnection *conn, HttpRequest &request);

 private:

  HttpCallback httpCallback_;       // std::function 回调函数　当接受到完整的http请求后处理
};

#endif  // MUDUO_NET_HTTP_HTTPSERVER_H
