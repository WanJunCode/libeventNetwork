#include "MysqlWrapper.h"
#include "../main/logcpp.h"
#include "MysqlPool.h"

MysqlWrapper::MysqlWrapper(MysqlConn *conn, MysqlPool *pool)
    :conn_(conn),
     pool_(pool){
    LOG_DEBUG("mysql wrapper get mysql conn\n");
}

MysqlWrapper::~MysqlWrapper(){
    pool_->release(conn_);
    LOG_DEBUG("mysql wrapper dtor : return mysql conn\n");
}

void MysqlWrapper::test(){
    LOG_DEBUG("myslq wrapper test\n");
}