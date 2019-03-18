#ifndef WANJUN_REDIS_CONFIG
#define WANJUN_REDIS_CONFIG

#include <string>

class RedisConfig{
public:
    RedisConfig(std::string ip = "localhost",unsigned int port = 6379,std::string passwd = "",unsigned int max = 10,unsigned int min = 1);

    std::string getIP(){
        return ip_;
    }
    unsigned int getPort(){
        return port_;
    }
    std::string getPasswd(){
        return passwd_;
    }
    unsigned int getMax(){
        return max_;
    }
    unsigned int getMin(){
        return min_;
    }

private:
    std::string ip_;
    unsigned int port_;
    std::string passwd_;
    unsigned int max_;
    unsigned int min_;
};
#endif // !WANJUN_REDIS_CONFIG