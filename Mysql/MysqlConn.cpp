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
