#include "MysqlPool.h"
#include "../main/logcpp.h"

// 忘记 将 conn 初始化为 NULL 导致abort
MysqlConn::MysqlConn()
	:conn(NULL){
	conn = mysql_init(conn);
}

MysqlConn::~MysqlConn(){
	if(conn){
		mysql_close(conn);
	}
	LOG_DEBUG("mysql dtor\n");
}

bool MysqlConn::connect(const char *host,const char *user,const char *passwd,const char *db,unsigned int port,const char *unix_socket,unsigned long clientflag){
	if(mysql_real_connect(conn,host,user,passwd,db,port,unix_socket,clientflag)){
		return true;
	}else{
		return false;
	}
}

MysqlConn::queryResult MysqlConn::execute(const char *sql){
		std::map<const std::string,std::vector<const char*> > results;
		if (conn) {
				if (mysql_query(conn,sql) == 0) {
					// 获得并存储返回的查询结果
					MYSQL_RES *res = mysql_store_result(conn);
					if (res) {
							MYSQL_FIELD *field;
							while ((field = mysql_fetch_field(res))) {
								results.insert(make_pair(field->name,std::vector<const char*>()));
							}
							MYSQL_ROW row;
							while ((row = mysql_fetch_row(res))) {
								unsigned int i = 0;
								for (std::map<const std::string,std::vector<const char*> >::iterator it = results.begin();
										it != results.end(); ++it) {
										(it->second).push_back(row[i++]);
								}     
							}
							mysql_free_result(res);
					}else{
							LOG_ERROR("mysql result [%s]\n",mysql_error(conn));
			}
			} else {
					LOG_ERROR("mysql query failure [%s]\n",mysql_error(conn));
			}
		} else {
				LOG_ERROR("mysql isn't connect [%s]\n",mysql_error(conn));
		}
		return results;
}


///////////////////////////////////////////////////////////////////////////
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
