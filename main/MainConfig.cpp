#include <jsoncpp/json/json.h>
// usr/include/jsoncpp/json/json.h

#include "MainConfig.h"
#include "logcpp.h"
#include "Tool.h"

MainConfig::MainConfig(const char *path){
    LOG_DEBUG("instructure of main config\n");
    loadFromFile(path);
}

MainConfig::~MainConfig(){
    LOG_DEBUG("destructure of main config\n");
}

void MainConfig::loadFromFile(const std::string path){
    char *text = textFileRead(path.c_str());
    if(text != NULL){
        LOG_DEBUG("read from file [%s]\n",text);

        parseFromJson(text);

        // 申请的空间是内置类型， malloc 申请的空间可以使用 free delete 释放
        free(text);
    }else{
        LOG_DEBUG("config json file read failure\n");
    }
}

bool MainConfig::parseFromJson(const std::string &json){
    try{
        // 可能导致内存泄露，单例模式
        Json::Value root;
        if(Json::Reader().parse(json,root)){

            if(root["port_"].isNumeric() == false){
                LOG_DEBUG("config json node[port_] is not numeric\n");
                return false;
            }
            port_ = root["port_"].asInt();

            if(root["bufferSize_"].isNumeric() == false){
                LOG_DEBUG("config json node[bufferSize_] is not numeric\n");                
                return false;
            }
            bufferSize_ = root["bufferSize_"].asInt();

            if(root["threadPoolSize_"].isNumeric() == false){
                LOG_DEBUG("config json node[threadPoolSize_] is not numeric\n");                
                return false;
            }
            threadPoolSize_ = root["threadPoolSize_"].asInt();

            if(root["IOThreads_"].isNumeric() == false){
                LOG_DEBUG("config json node[IOThreads_] is not numeric\n");                
                return false;
            }
            IOThreads_ = root["IOThreads_"].asInt();

            if(root["backlog_"].isNumeric() == false){
                LOG_DEBUG("config json node[backlog_] is not numeric\n");                
                return false;
            }
            backlog_ = root["backlog_"].asInt();

            if(root["logFileOutput_"].isString() == false){
                LOG_DEBUG("config json node[logFileOutput_] is not string\n");
                return false;    
            }
            logFileOutput_ = root["logFileOutput_"].asString();

            return true;
        }else{
            LOG_DEBUG("reader parse failed\n");
        }
    }catch(std::exception &e){
        LOG_DEBUG("json parse failded:%s\n",e.what());
    }
    return true;
}