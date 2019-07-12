#include "MysqlWrapper.h"
#include "MysqlPool.h"
#include "../main/logcpp.h"

MysqlWrapper::MysqlWrapper(MysqlConn *conn, MysqlPool *pool)
    :conn_(conn),
     pool_(pool){
}

MysqlWrapper::~MysqlWrapper(){
    pool_->release(conn_);
    // LOG_DEBUG("mysql wrapper dtor : return mysql conn\n");
}

void MysqlWrapper::test(){
    LOG_DEBUG("myslq wrapper test\n");
}

const char *MysqlWrapper::createAccount(const char *phone){
    std::ostringstream oss;
    oss<<"INSERT INTO account(phone) VALUES(\""<<phone<<"\");";
    bool ok;
    conn_->execute(oss.str().data(),ok);
    if(ok){
        // 清空操作
        oss.str("");
        oss<<"SELECT id FROM account WHERE phone LIKE \""<<phone<<"\";";
        // 插入数据成功，获得新建账户的id值
        auto result =conn_->execute(oss.str().data(),ok);
        if(ok){
            return result["id"][0];
        }
    }
    return NULL;
}