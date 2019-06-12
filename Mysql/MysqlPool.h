// #ifndef MYSQLPP_MYSQL_HEADERS_BURIED
// #define MYSQLPP_MYSQL_HEADERS_BURIED

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

// class MysqlPool : public mysqlpp::ConnectionPool{
// private:
//     // Our connection parameters
//     const std::string server_,db_,user_, password_;
//     unsigned int port_;
//     unsigned int keeplive_;
//     unsigned int conns_max_;
//     bool loop;

//     // Number of connections currently in use
//     AtomicInt32 conns_in_use_;
//     std::condition_variable             condition_;
//     std::mutex                          mutex_;
//     std::vector<mysqlpp::Connection*>   resumMysql_;        // 可复用的 mysql connection
//     Queue_s<std::string >               sqlString_;         // sql 任务队列

// public:
//     // 单例模式
//     static MysqlPool *getInstance(){
//         static MysqlPool pool("localhost",3306,"test_db","root","wanjun",5,20);
//         return &pool;
//     }

//     void exit(){
//         loop = false;
//         sqlString_.wake_all();
//     }

//     MysqlPool(const std::string &host_addr,
//                         unsigned int port,
//                         const std::string &db_name,
//                         const std::string &user,
//                         const std::string &password,
//                         unsigned int keeplive,
//                         unsigned int maxConn);
//     ~MysqlPool();

//     //Override
// public:
//     // in-use connection limiting: wait to return a connection until there are a reasonably low number in use already.
//     // Can't do this in create() because we're interested in connections actually in use, not those created.  Also note that
//     // we keep our own count; ConnectionPool::size() isn't the same!
//     // std::shared_ptr<MysqlWrapper> grabConn() {
//     //     return std::make_shared<MysqlWrapper>(this);
//     // };

//     void runInThread();
// public:
//     // in-use connection limiting: wait to return a connection until there are a reasonably low number in use already.
//     mysqlpp::Connection* grab() override;

//     // Other half of in-use conn count limit
//     void release(const mysqlpp::Connection* conn) override;

// //Override
// protected:
//     // Superclass overrides 基类覆盖
//     mysqlpp::Connection* create() override;
//     void destroy(mysqlpp::Connection* conn) override;
//     unsigned int max_idle_time() override{
//         return keeplive_; //max idle time
//     }
// };

#endif // !MYSQLPP_MYSQL_HEADERS_BURIED
// #endif // !MYSQL_MYSQLPOOL
