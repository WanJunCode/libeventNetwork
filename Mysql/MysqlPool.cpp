#include "MysqlPool.h"
#include "../main/logcpp.h"

/*
 *有参的单例函数，用于第一次获取连接池对象，初始化数据库信息。
 */
MysqlPool* MysqlPool::getInstance() {
	static MysqlPool mysqlpool_object;           //类的对象
	return &mysqlpool_object;
}
			 
MysqlPool::MysqlPool() {
	//初始化数据库
	if (0 != mysql_library_init(0, NULL, NULL)){
		LOG_WARN("mysql library init failure\n");
		abort();
	}
}

MysqlPool::~MysqlPool(){
	while(!stack_.empty()){
		auto tmp = stack_.top();
		stack_.pop();
		delete tmp;
	}
	LOG_DEBUG("mysql pool dtor\n");
	// mysql pool 始终会泄露内存的原因
	mysql_library_end();
}


/*
 *配置数据库参数
 */
void MysqlPool::setParameter( const char *mysqlhost,const char *mysqluser,const char *mysqlpwd,const char *databasename,unsigned int  port,const char *socket,unsigned long client_flag,unsigned int  max_connect ) {
	_mysqlhost    = mysqlhost;
	_mysqluser    = mysqluser;
	_mysqlpwd     = mysqlpwd;
	_databasename = databasename;
	_port         = port;
	_socket       = socket;
	_client_flag  = client_flag;
	MAX_CONNECT   = max_connect;

	// 预先存入 多个可用的连接
	for(size_t i = 0;i<5;i++){
		auto tmp = createOneConnect();
		stack_.push(tmp);
	}
}
	
MysqlConn *MysqlPool::createOneConnect(){
	LOG_DEBUG("mysql pool create new mysql conn\n");
	MysqlConn *tmp = new MysqlConn();
	if(tmp->connect(_mysqlhost,_mysqluser,_mysqlpwd,_databasename,_port,_socket,_client_flag)){
		return tmp;
	}
	return NULL;
}

MysqlConn *MysqlPool::grab(){
	std::unique_lock<std::mutex> locker(mutex_);
	if(stack_.empty()){
		return createOneConnect();
	}else{
		auto tmp = stack_.top();
		stack_.pop();
		return tmp;
	}
}

void MysqlPool::release(MysqlConn *conn){
	std::unique_lock<std::mutex> locker(mutex_);
	if(stack_.size()>MAX_CONNECT){
		LOG_DEBUG("mysql pool stack full\n");
		delete conn;
	}else{
		LOG_DEBUG("mysql pool reuse mysql conn\n");
		stack_.push(conn);
	}
}

std::shared_ptr<MysqlWrapper> MysqlPool::getMysqlWrapper(){
	auto conn = grab();
	return std::make_shared<MysqlWrapper>(conn,this);
}
