
/*
mysql pool 连接池,使用两种方式完成 sql 语句的执行,获得 sql连接阻塞的执行相关sql语句
使用 pool 异步执行sql语句,适用于不需要获得返回值的情况

数据库连接池设计
同步 获得一个 conn 使用,使用完后返回 目前自己回收 release
grab 获取连接时不够 则创建新的连接 返回,在回收的时候delete 删除

使用 sqlString 安全队列
异步 另起一个线程处理 sql

*/

#ifndef MYSQL_MYSQLPOOL
#define MYSQL_MYSQLPOOL

#include <mysql/mysql.h>
#include <stack>
#include <map>
#include "../base/Atomic.h"
#include "../Cedis/Queue_s.h"

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


class MysqlPool {
public:
	MysqlPool();
	static MysqlPool* getInstance();              //单列模式获取本类的对象
		// 开放的api
	~MysqlPool();
	void setParameter( const char*   _mysqlhost,
											 const char*   _mysqluser,
											 const char*   _mysqlpwd,
											 const char*   _databasename,
											 unsigned int  _port = 0,
											 const char*   _socket = NULL,
											 unsigned long _client_flag = 0,
											 unsigned int  MAX_CONNECT = 50 );              //设置数据库参数
	MysqlConn *grab();
	void release(MysqlConn *conn);

private:
	MysqlConn *createOneConnect();
private:
	const char*   _mysqlhost;                     //mysql主机地址
	const char*   _mysqluser;                     //mysql用户名
	const char*   _mysqlpwd;                      //mysql密码
	const char*   _databasename;                  //要使用的mysql数据库名字
	unsigned int  _port;                          //mysql端口
	const char*   _socket;                        //可以设置成Socket or Pipeline，通常设置为NULL
	unsigned long _client_flag;                   //设置为0
	unsigned int  MAX_CONNECT;                    //同时允许最大连接对象数量

	Queue_s<std::string> 	sqlString_;				// 异步执行的任务队列
	std::stack<MysqlConn *> 	stack_;					// 复用的 mysql 连接
	AtomicInt64  			connect_count;                  //目前连接池的连接对象数量
	std::mutex  			mutex_;                          //连接池锁
};


#endif // !MYSQL_MYSQLPOOL