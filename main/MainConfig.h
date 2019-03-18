#ifndef WANJUN_MAIN_CONFIJ_H
#define WANJUN_MAIN_CONFIJ_H

#include <string>
#include <memory>
#include <jsoncpp/json/json.h>
#include "../Cedis/RedisConfig.h"
#define CONFIG_PATH "etc/config.json"

class MainConfig{
public:
    MainConfig(const char *path);
    ~MainConfig();

    void loadFromFile(const std::string path);
    bool parseFromJson(const std::string &json);

    bool getRedis(std::string config);

    size_t getPort(){
        return port_;
    }
    size_t getBufferSize(){
        return bufferSize_;
    }
    size_t getThreadPoolSize(){
        return threadPoolSize_;
    }
    size_t getIOThreadSize(){
        return IOThreads_;
    }
    size_t getBacklog(){
        return backlog_;
    }

    const std::string logcppfilePath() const{
        return logFileOutput_;
    }
    
    std::shared_ptr<RedisConfig> redisConfig(){
        return redisConfig_;
    }

private:
    size_t port_;
    size_t bufferSize_;
    size_t threadPoolSize_;
    size_t IOThreads_;                 // io线程数量
    size_t backlog_;
    std::string logFileOutput_;    // log日志文件路径

    std::shared_ptr<RedisConfig> redisConfig_;
};

#endif // !WANJUN_MAIN_CONFIJ_H
