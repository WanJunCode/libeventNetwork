#include <jsoncpp/json/json.h>
// usr/include/jsoncpp/json/json.h

#include "MainConfig.h"
#include "log.h"
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
        delete text;
    }
}

bool MainConfig::parseFromJson(const std::string &json){
    try{
        Json::Value root;
        if(Json::Reader().parse(json,root)){
            if(!root["ThreadPools"].isNumeric()){
                LOG_DEBUG("config json node[ThreadPools] is not numeric\n");
                return false;
            }
            numOfThreadPool = root["ThreadPools"].asInt();
            LOG_DEBUG("number of thread pool is [%d]\n",numOfThreadPool);

            return true;
        }else{
            LOG_DEBUG("reader parse failed\n");
        }
    }catch(std::exception &e){
        LOG_DEBUG("json parse failded:%s\n",e.what());
    }
    return true;
}