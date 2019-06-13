#ifndef MYSQL_MYSQLCONN_H
#define MYSQL_MYSQLCONN_H

#include <map>
#include <mysql/mysql.h>
#include <vector>

class MysqlConn{
private:
	MYSQL *conn;
public:
	typedef std::map<const std::string,std::vector<const char*> > queryResult;
	MysqlConn();
	~MysqlConn();
	bool connect(const char *host,const char *user,const char *passwd,const char *db,unsigned int port,const char *unix_socket,unsigned long clientflag);
	queryResult execute(const char *sql);
};

#endif // !MYSQL_MYSQLCONN_H