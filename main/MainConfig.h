#ifndef WANJUN_MAIN_CONFIJ_H
#define WANJUN_MAIN_CONFIJ_H

#include <string>
#define CONFIG_PATH "etc/config.json"

class MainConfig{
public:
    MainConfig(const char *path);
    ~MainConfig();

    void loadFromFile(const std::string path);
    bool parseFromJson(const std::string &json);


    int getThreadPools() const{
        return numOfThreadPool;
    }
    int getIOThreads() const{
        return numOfIOThreads;
    }
    const std::string logcppfilePath() const{
        return path_for_logcppfile;
    }
    
private:
    int numOfThreadPool;                // 线程池数量
    int numOfIOThreads;                 // io线程数量
    std::string path_for_logcppfile;    // log日志文件路径
};

#endif // !WANJUN_MAIN_CONFIJ_H