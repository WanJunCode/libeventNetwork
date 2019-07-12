#include "MysqlConn.h"
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
	// LOG_DEBUG("mysql dtor\n");
}

bool MysqlConn::connect(const char *host,const char *user,const char *passwd,const char *db,unsigned int port,const char *unix_socket,unsigned long clientflag){
	if(mysql_real_connect(conn,host,user,passwd,db,port,unix_socket,clientflag)){
		return true;
	}else{
		return false;
	}
}

// 没有判断是否执行正确的操作
// 需要重新设计一下
// 返回一个唯一指针？　通过判断指针是否存在　是否执行成功
MysqlConn::queryResult MysqlConn::execute(const char *sql, bool& ok){
	ok = true;
	std::map<const std::string,std::vector<const char*> > results;
	if (conn) {
		// 查询结果存储在　conn
		if (mysql_query(conn,sql) == 0) {
			// 获得并存储返回的查询结果
			MYSQL_RES *res = mysql_store_result(conn);
			if (res) {
				// 结果列名称
				MYSQL_FIELD *field;
				while ((field = mysql_fetch_field(res))) {
					// 保存获得的结果域名
					results.insert(make_pair(field->name,std::vector<const char*>()));
				}
				// 结果行
				MYSQL_ROW row;
				// while 循环，每次获得一行数据
				while ((row = mysql_fetch_row(res))) {
					unsigned int i = 0;
					for (auto it = results.begin();it != results.end(); ++it) {
						// std::vector<const char*>
						(it->second).push_back(row[i++]);
					}
				}
				mysql_free_result(res);
			}else{
				// 这里是正确执行的，插入语句执行正确，但是没有返回值
				LOG_ERROR("mysql result [%s]\n",mysql_error(conn));
			}
		} else {
			ok = false;
			LOG_ERROR("mysql query failure [%s]\n",mysql_error(conn));
		}
	} else {
		ok = false;
		LOG_ERROR("mysql isn't connect [%s]\n",mysql_error(conn));
	}
	return results;
}
