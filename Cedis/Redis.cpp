#include "Redis.h"
#include "RedisPool.h"
#include <iostream>
#include "../log.h"
using namespace std;

int Redis::count=0;

// 构造函数
Redis::Redis(const std::string& host, const unsigned port,const std::string& pass){
    Connect(host,port,pass);
    count++;
}

void Redis::Connect(const std::string& host, const unsigned int port,const std::string& pass){
    conn_ = redisConnect(host.c_str(), port);
    if (conn_->err != REDIS_OK){
        redisFree(conn_);
        LOG_DEBUG("fail to connect redis server.\n");
    }else{
        redisReply *myreply=(redisReply *)redisCommand(conn_,"AUTH %s",pass.data());
        freeReplyObject(myreply);
        useable=true;
    }
}

Redis::~Redis(){
    redisFree(conn_);
    count--;
}

void Redis::append(const std::vector<std::string> &commands){
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
    int ret = redisAppendCommandArgv(conn_, static_cast<int>(commands.size()), argv.data(), argvlen.data());
    if (ret != REDIS_OK){
        LOG_DEBUG("append redis command fail\n");
    }
}

Reply Redis::get_reply(){
    redisReply *rep;
    int error = redisGetReply(conn_, reinterpret_cast<void**>(&rep));
    if (error != REDIS_OK){
        LOG_DEBUG("get_reply fail\n");
    }
    Reply ret(rep);
    freeReplyObject(rep);
    if (ret.type() == Reply::type_t::ERROR &&
		(ret.str().find("READONLY") == 0) ){
        LOG_DEBUG("get_reply type fail\n");
    }
    return ret;
}

// 获得多个返回结果
std::vector<Reply> Redis::get_replies(unsigned int count){
    std::vector<Reply> ret;
    for (unsigned int i=0; i < count; ++i){
        ret.push_back(get_reply());
    }
    return ret;
}

bool Redis::is_valid() const{
    return conn_->err == REDIS_OK;
}

Reply Redis::run(const std::vector<std::string>& args){
    std::unique_lock<std::mutex> locker(mutex_);            // 上锁，防止多个线程的争夺
    if(!useable){
        Reply r = Reply();
        r.setDisconn(true);
        return r;
    }
    // 断开连接后 ping 返回 null
    if(!ping()){
        useable = false;
        Reply r = Reply();
        pool_->move(this);
        r.setDisconn(true);
        return r;
    }

    append(args);
    return get_reply();
}

Reply Redis::executeCommand(const char *format, ...){
    std::unique_lock<std::mutex> locker(mutex_);            // 上锁，防止多个线程的争夺

    if(!useable){
        Reply r = Reply();
        r.setDisconn(true);
        return r;
    }

    // 使用 hiredis 方法
	va_list ap;
	va_start(ap, format);
	redisReply* r = (redisReply*)redisvCommand(conn_, format, ap);
	va_end(ap);

    if(r==NULL){
        useable = false;
        Reply r = Reply();
        pool_->move(this);
        r.setDisconn(true);
        return r;
    }

    Reply ret(r);
    freeReplyObject(r);
	return ret;
}

bool Redis::ping(){
    redisReply *myreply=(redisReply *)redisCommand(conn_,"PING");
    if(myreply==NULL){
        return false;
    }else{
        freeReplyObject(myreply);
        return true;
    }
}
