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
            if(root["workers"].isNumeric() == false){
                LOG_DEBUG("config json node[workers] is not numeric\n");
                return false;
            }
            numOfThreadPool = root["workers"].asInt();
            if(root["iothreads"].isNumeric() == false){
                LOG_DEBUG("config json node[iothreads] is not numeric\n");                
                return false;
            }
            numOfIOThreads = root["iothreads"].asInt();

            if(root["logFileOutput"].isString() == false){
                LOG_DEBUG("config json node[logFileOutput] is not string\n");
                return false;    
            }
            path_for_logcppfile = root["logFileOutput"].asString();


            return true;
        }else{
            LOG_DEBUG("reader parse failed\n");
        }
    }catch(std::exception &e){
        LOG_DEBUG("json parse failded:%s\n",e.what());
    }
    return true;
}