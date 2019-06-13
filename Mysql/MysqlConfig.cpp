#include "MysqlConfig.h"
#include "../main/logcpp.h"

MysqlConfig::MysqlConfig(std::string host,
                std::string user,
                std::string passwd,
	            std::string db,
	            unsigned int port,
	            unsigned int maxconn){
    host_ = host;
    user_ = user;
    passwd_ = passwd;
    db_ = db;
    port_ = port;
    maxconn_ = maxconn;
}

MysqlConfig::~MysqlConfig(){
    LOG_DEBUG("mysql config dtor\n");
}
