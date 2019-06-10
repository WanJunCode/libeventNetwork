#include "ChatMessage.h"

Package *ChatMessage::process(Json::Value &root){
    try{
        std::string id = root["id"].asString();
        std::string message = root["message"].asString();
        LOG_DEBUG("注册账户 id[%s] message[%s]\n",id.data(),message.data());
    }catch (std::exception &e) {
        LOG_ERROR("json error:%s", e.what());
    }
    return nullptr;
}