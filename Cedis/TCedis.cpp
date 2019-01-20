#include "TCedis.h"
#include "TCedisPool.h"
#include <iostream>
using namespace std;

// 构造函数
TCedis::TCedis(const std::string& host, const unsigned port,const std::string& pass){
    Connect(host,port,pass);
}


void TCedis::Connect(const std::string& host, const unsigned int port,const std::string& pass){
    c = redisConnect(host.c_str(), port);
    if (c->err != REDIS_OK){
        redisFree(c);
        printf("fail to connect redis server.\n");
    }else{
        redisCommand(c,"AUTH %s",pass.data());
        useable=true;
    }
}

TCedis::~TCedis(){
    redisFree(c);
}

void TCedis::append(const std::vector<std::string> &commands){
    std::vector<const char*> argv;
    argv.reserve(commands.size());
    std::vector<size_t> argvlen;
    argvlen.reserve(commands.size());

    // std::string ==> [ const char* & size_t ]

    for (std::vector<std::string>::const_iterator it = commands.begin(); it != commands.end(); ++it) {
        // string.c_str() & string.size()
        argv.push_back(it->c_str());
        argvlen.push_back(it->size());
    }
    // std::vector.data()
    int ret = redisAppendCommandArgv(c, static_cast<int>(commands.size()), argv.data(), argvlen.data());
    if (ret != REDIS_OK){
        printf("append redis command fail\n");
    }
}

reply TCedis::get_reply(){
    redisReply *r;
    int error = redisGetReply(c, reinterpret_cast<void**>(&r));
    if (error != REDIS_OK){
        printf("get_reply fail\n");
    }
    reply ret(r);
    freeReplyObject(r);
    if (ret.type() == reply::type_t::ERROR &&
		(ret.str().find("READONLY") == 0) ){
        printf("get_reply type fail\n");
    }
    return ret;
}

// 获得多个返回结果
std::vector<reply> TCedis::get_replies(unsigned int count){
    std::vector<reply> ret;
    for (unsigned int i=0; i < count; ++i){
        ret.push_back(get_reply());
    }
    return ret;
}

bool TCedis::is_valid() const{
    return c->err == REDIS_OK;
}

reply TCedis::run(const std::vector<std::string>& args){
    std::unique_lock<std::mutex> locker(mutex_);            // 上锁，防止多个线程的争夺
    if(!useable){
        reply r = reply();
        r.setDisconn(true);
        return r;
    }
    // 断开连接后 ping 返回 null
    if(!ping()){
        useable = false;
        reply r = reply();
        pool_->move(self);
        r.setDisconn(true);
        return r;
    }

    append(args);
    return get_reply();
}

reply TCedis::executeCommand(const char *format, ...){
    std::unique_lock<std::mutex> locker(mutex_);            // 上锁，防止多个线程的争夺

    if(!useable){
        reply r = reply();
        r.setDisconn(true);
        return r;
    }

    // 使用 hiredis 方法
	va_list ap;
	va_start(ap, format);
	redisReply* r = (redisReply*)redisvCommand(c, format, ap);
	va_end(ap);

    if(r==NULL){
        useable = false;
        reply r = reply();
        pool_->move(self);
        r.setDisconn(true);
        return r;
    }

    reply ret(r);
    freeReplyObject(r);
	return ret;
}

bool TCedis::ping(){
    redisReply *myreply=(redisReply *)redisCommand(c,"PING");
    if(myreply==NULL){
        return false;
    }else{
        freeReplyObject(myreply);
        return true;
    }
}
