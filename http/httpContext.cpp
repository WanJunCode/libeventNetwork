#include "../base/Buffer.h"
#include "httpContext.h"

// 解析请求数据报头
bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    // 开头到第一个空格之间是　method
    if (space != end && request_.setMethod(start, space))
    {
        start = space+1;
        space = std::find(start, end, ' ');
        // 找到第二个　空格
        if (space != end)
        { 
            // 第一个空格到第二个空格之间是否带有参数 ?
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                // start --- question --- space  
                //      path    ?    query
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else
            {
                // start --- space
                //      path
                request_.setPath(start, space);
            }

            // start 记住第二个空格的位置
            start = space+1;
            succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
            // end 到　start 长度为　８　，并且等于 "HTTP/1."

            if (succeed)
            {
                // http 1.1
                if (*(end-1) == '1'){
                    request_.setVersion(HttpRequest::kHttp11);
                }else if (*(end-1) == '0'){
                    // http 1.0
                    request_.setVersion(HttpRequest::kHttp10);
                }else{
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

// 解析　http 请求
// return false if any error
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime){

    bool ok = true;
    bool hasMore = true;
    while (hasMore){
        if (state_ == kExpectRequestLine){
            // 找到第一个　crlf 并处理　requestline
            const char* crlf = buf->findCRLF();
            if (crlf){
                // 开头到第一个　crlf 都属于　requestline
                ok = processRequestLine(buf->peek(), crlf);
                if (ok){
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);   // 丢弃
                    state_ = kExpectHeaders;
                }else{
                    // requestline　解析失败
                    hasMore = false;
                }
            }else{
                    // 没有换行符
                    hasMore = false;
            }
        }else if (state_ == kExpectHeaders){
            // while 循环获得　键值对, 响应头域每行都以　CRLF 结束
            const char* crlf = buf->findCRLF();
            if (crlf){
                // 从中找到　: 字符
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf){
                    // 添加　http 数据包报头
                    request_.addHeader(buf->peek(), colon, crlf);
                }else{
                    // 没有找到　: 字符，不包含数据包报头的键值对
                    // empty line, end of header
                    // FIXME:
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);   //　丢弃
            }else{
                hasMore = false;
            }
        }else if (state_ == kExpectBody){
            // FIXME:
        }
    }
    return ok;
}
