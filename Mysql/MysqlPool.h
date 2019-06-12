// #ifndef MYSQLPP_MYSQL_HEADERS_BURIED
// #define MYSQLPP_MYSQL_HEADERS_BURIED

#ifndef MYSQL_MYSQLPOOL
#define MYSQL_MYSQLPOOL

#include <mysql++/mysql++.h>
#include "../base/Atomic.h"
#include "../Cedis/Queue_s.h"

class MysqlPool : public mysqlpp::ConnectionPool{
private:
    // Our connection parameters
    const std::string server_,db_,user_, password_;
    unsigned int port_;
    unsigned int keeplive_;
    unsigned int conns_max_;

    // Number of connections currently in use
    AtomicInt32 conns_in_use_;
    
    std::condition_variable             condition_;
    std::mutex                          mutex_;
    std::vector<mysqlpp::Connection*>   resumMysql_;
    Queue_s<std::string >               sqlString_;

public:
    MysqlPool(const std::string &host_addr,
                        unsigned int port,
                        const std::string &db_name,
                        const std::string &user,
                        const std::string &password,
                        unsigned int keeplive,
                        unsigned int maxConn);
    ~MysqlPool();

    //Override
public:
    // in-use connection limiting: wait to return a connection until there are a reasonably low number in use already.
    // Can't do this in create() because we're interested in connections actually in use, not those created.  Also note that
    // we keep our own count; ConnectionPool::size() isn't the same!
    // std::shared_ptr<MysqlWrapper> grabConn() {
    //     return std::make_shared<MysqlWrapper>(this);
    // };

    void runInThread();
public:
    // in-use connection limiting: wait to return a connection until there are a reasonably low number in use already.
    mysqlpp::Connection* grab() override;

    // Other half of in-use conn count limit
    void release(const mysqlpp::Connection* conn) override;

//Override
protected:
    // Superclass overrides
    mysqlpp::Connection* create() override;
    void destroy(mysqlpp::Connection* conn) override;
    unsigned int max_idle_time() override{
        return keeplive_; //max idle time
    }
};

#endif // !MYSQLPP_MYSQL_HEADERS_BURIED
// #endif // !MYSQL_MYSQLPOOL
