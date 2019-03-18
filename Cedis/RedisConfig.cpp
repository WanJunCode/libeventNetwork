#include "RedisConfig.h"

RedisConfig::RedisConfig(std::string ip,unsigned int port,std::string passwd,unsigned int max,unsigned int min)
    :ip_(ip),
     port_(port),
     passwd_(passwd),
     max_(max),
     min_(min){

}