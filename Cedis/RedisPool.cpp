#include "RedisPool.h"
#include "RedisConfig.h"
#include <algorithm>
#include <hiredis/hiredis.h>
#include <atomic>

#include "../main/logcpp.h"

RedisPool::RedisPool(	std::string server,
                		unsigned int port,
						std::string password,
                		unsigned int conns_max,
						unsigned int conns_min)
	:server_(server),
	port_(port),
	password_(password),
	conns_max_(conns_max),
	conns_min_(conns_min),
	stop(false){
}

RedisPool::RedisPool(std::shared_ptr<RedisConfig> config)
	:stop(false){
	if(config){
		server_ = config->getIP();
		port_ = config->getPort();
		password_ = config->getPasswd();
		conns_max_ = config->getMax();
		conns_min_ = config->getMin();
	}else{
		LOG_FATAL("redis Pool config not give\n");
		assert(false);
	}
}

void RedisPool::init(){
	for(unsigned int i=0;i<conns_min_;++i){
		// 使用智能共享指针管理 redis 连接
		auto redis = make_shared<Redis>(server_,port_,password_);
		if(redis->is_valid()){								// 判断该连接是否可用
			redis->setUseable();								// 设置为可用
			// shared_ptr -> weak_ptr
			redis->attach(shared_from_this());				// 给连接绑定连接池
			redisVec.push_back(redis);
		}else{
    		LOG_ERROR("fail to create redis[%d] connection\n",i);
		}
	}
}

RedisPool::~RedisPool(){
	LOG_DEBUG("destructure of cedis pool\n");
}

// 使用RR方法获得redis连接
// 如果出现所有连接都不可用的情况怎么办？
std::shared_ptr<Redis> RedisPool::grabCedis(){
	std::lock_guard<std::mutex> locker(mutex_);
	static std::atomic_llong idx(1);
	// 有可能　llong 都使用完毕的情况
	int index = (idx++ % redisVec.size());
	while(!redisVec[index]->getuseable()){
		index = (idx++ % redisVec.size());
	}
	LOG_DEBUG("grab redis [%d]\n",index);
	return redisVec[index];
}

// 将queue中的不可用redis重连
void RedisPool::reuseCedis(){
	bool reuse=false;
	// 结束连接池时　queue.weak_all() ,返回的　hasCedis 为　false
	auto tmp = queue.pop_front(reuse);			// 此处是阻塞的，线程安全的 队列
	if(reuse){
		// 这里使用 ping 会出错
		tmp->reConnect();
		tmp->setUseable();
	}
}