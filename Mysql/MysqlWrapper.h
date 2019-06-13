#ifndef MYSQL_MYSQLWRAPPER_H
#define MYSQL_MYSQLWRAPPER_H

// 更好的使用 mysql pool 获得的 mysql conn,防止内存泄露
#include "MysqlConn.h"

class MysqlPool;
class MysqlWrapper{
private:
    // 内部使用指针 or 引用
    MysqlConn *conn_;
    MysqlPool *pool_;
public:
    MysqlWrapper(MysqlConn *conn, MysqlPool *pool);
    ~MysqlWrapper();

    void test();
};


#endif // !MYSQL_MYSQLWRAPPER_H