#include "RedisPool.h"
#include <algorithm>
#include <hiredis/hiredis.h>
#include <atomic>

#include "../main/log.h"

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
	init();
}

void RedisPool::init(){
	for(unsigned int i=0;i<conns_min_;++i){	
		auto conn = new Redis(server_,port_,password_);
		if(conn->is_valid()){								// 判断该连接是否可用
			conn->setUseable();								// 设置为可用
			conn->attach(this);								// 给连接绑定连接池
			redis_vec.push_back(conn);					// 插入cedisVector队尾部
		}else{
    		LOG_DEBUG("fail to create a conn\n");
		}
	}
}

RedisPool::~RedisPool(){
	while(!redis_vec.empty()){
		auto tmp = redis_vec.back();
		// 防止循环引用
		redis_vec.pop_back();
		delete tmp;
	}
	if(Redis::count>0){
		LOG_DEBUG("size of redis leak [%d]\n",Redis::count);
	}
	LOG_DEBUG("destructure of cedis pool\n");
}

Redis *RedisPool::grabCedis(){
	std::unique_lock<std::mutex> locker(mutex_);
	static std::atomic_llong mid(1);
	// 有可能　llong 都使用完毕的情况
	int index = (mid++ % redis_vec.size());
	while(!redis_vec[index]->getuseable()){
		index = (mid++ % redis_vec.size());
	}
	return redis_vec[index];
}

void RedisPool::reuseCedis(){
	bool hasCedis=false;
	// 结束连接池时　queue.weak_all() ,返回的　hasCedis 为　false
	auto tmp = queue.pop_front(hasCedis);			// 此处是阻塞的，线程安全的 队列
	if(hasCedis){
		// 这里使用 ping 会出错
		tmp->reConnect();
		tmp->setUseable();
	}
}