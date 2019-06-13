#ifndef MYSQL_MYSQLCONFIG_H
#define MYSQL_MYSQLCONFIG_H

#include <string>

class MysqlConfig{
private:
    std::string host_;
	std::string user_;
	std::string passwd_;
	std::string db_;
	unsigned int port_;
	unsigned int maxconn_;

public:
    MysqlConfig(std::string host,
                std::string user,
                std::string passwd,
	            std::string db,
	            unsigned int port,
	            unsigned int maxconn);

    ~MysqlConfig();
};

#endif // !MYSQL_MYSQLCONFIG_H