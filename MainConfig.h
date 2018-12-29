#ifndef WANJUN_MAIN_CONFIJ_H
#define WANJUN_MAIN_CONFIJ_H

#include <string>
#define CONFIG_PATH "../etc/config.json"

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

    const char *logfilePath() const{
        return path_for_logfile;
    }
private:
    int numOfThreadPool;
    int numOfIOThreads;
    char *path_for_logfile;
};

#endif // !WANJUN_MAIN_CONFIJ_H