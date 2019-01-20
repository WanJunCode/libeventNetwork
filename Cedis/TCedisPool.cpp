#include "TCedisPool.h"
#include <algorithm>
#include <hiredis/hiredis.h>
#include <atomic>

TCedisPool::TCedisPool(	std::string server,
                		unsigned int port,
						std::string password,
                		unsigned int conns_max,
						unsigned int conns_min)
	:server_(server),
	port_(port),
	password_(password),
	conns_max_(conns_max),
	conns_min_(conns_min){
	init();
}

void TCedisPool::init(){
	for(unsigned int i=0;i<conns_min_;i++){	
		TCedis::ptr_t conn = TCedis::create(server_,port_,password_);
		if(conn->is_valid()){								// 判断该连接是否可用
			conn->setUseable();								// 设置为可用
			conn->attach(this);								// 给连接绑定连接池
			conn->self=conn;
			cedisVector.push_back(conn);					// 插入cedisVector队尾部
		}else{
    		printf("fail to create a conn\n");
		}
	}
}


TCedisPool::~TCedisPool(){

}

TCedis::ptr_t TCedisPool::grabCedis(){
	std::unique_lock<std::mutex> locker(mutex_);
	static std::atomic_llong mid(1);
	// 有可能　llong 都使用完毕的情况
	int index = (mid++ % cedisVector.size());
	while(!cedisVector[index]->getuseable()){
		index = (mid++ % cedisVector.size());
	}
	return cedisVector[index];
}

void TCedisPool::reuseCedis(){
	bool hasCedis=false;
	TCedis::ptr_t tmp = queue.pop_front(hasCedis);			// 此处是阻塞的，线程安全的 队列
	if(hasCedis){
		// 这里使用 ping 会出错
		tmp->reConnect();
		tmp->setUseable();
	}
}