#ifndef MYSQL_MYSQLWRAPPER_H
#define MYSQL_MYSQLWRAPPER_H

// 更好的使用 mysql pool 获得的 mysql conn,防止内存泄露
// MysqlWrapper 表示 conn 和 pool 的关系,不可被复制
#include "MysqlConn.h"
#include <memory>

class MysqlPool;
class MysqlWrapper : public noncopyable{
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