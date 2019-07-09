
#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H

#include "../base/copyable.h"
#include "../Util/Timestamp.h"
#include "../base/Types.h"

#include <map>
#include <assert.h>
#include <stdio.h>

// http　请求
class HttpRequest : public copyable
{
 public:
  enum Method{
    kInvalid, kGet, kPost, kHead, kPut, kDelete
  };

  enum Version{
    kUnknown, kHttp10, kHttp11
  };

 private:
  Method      method_;                     //　请求的方法
  Version     version_;                   // 版本
  string      path_;                       // 路径
  string      query_;                      // 查询
  Timestamp   receiveTime_;             // 接收时间
  std::map<string, string> headers_;  // 数据包报头

 public:
  HttpRequest()
    : method_(kInvalid),
      version_(kUnknown){
  }

  // 设置版本
  void setVersion(Version v)  {
    version_ = v;
  }

  Version getVersion() const
  { return version_; }

  bool setMethod(const char* start, const char* end){
    // 根据　start end 两个指针指向的内容确定　http 请求的类型
    assert(method_ == kInvalid);
    string m(start, end);
    if (m == "GET"){
      method_ = kGet;
    }
    else if (m == "POST"){
      method_ = kPost;
    }
    else if (m == "HEAD"){
      method_ = kHead;
    }
    else if (m == "PUT"){
      method_ = kPut;
    }
    else if (m == "DELETE"){
      method_ = kDelete;
    }
    else{
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const
  { return method_; }

  // 获得　method 对应的　字符串
  const char* methodString() const
  {
    const char* result = "UNKNOWN";
    switch(method_)
    {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void setPath(const char* start, const char* end){
    path_.assign(start, end);
  }

  const string& path() const
  { return path_; }

  void setQuery(const char* start, const char* end){
    query_.assign(start, end);
  }

  const string& query() const
  { return query_; }

  // 设置接收时间
  void setReceiveTime(Timestamp t)
  { receiveTime_ = t; }

  Timestamp receiveTime() const
  { return receiveTime_; }

  // start --- colon --- end
  //       key : value
  void addHeader(const char* start, const char* colon, const char* end){
    string field(start, colon);
    ++colon;
    // 忽略空格字符
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }
    string value(colon, end);
    // 取出　value 后的　空格
    while (!value.empty() && isspace(value[value.size()-1]))
    {
      value.resize(value.size()-1);
    }
    // std::map 增加一个键值对
    printf("key [%s] : value [%s]\n",field.data(),value.data());
    headers_[field] = value;
  }

  string getHeader(const string& field) const{
    string result;
    std::map<string, string>::const_iterator it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }

  const std::map<string, string>& headers() const{ return headers_; }

  void swap(HttpRequest& that){
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
  }

};

#endif  // MUDUO_NET_HTTP_HTTPREQUEST_H
