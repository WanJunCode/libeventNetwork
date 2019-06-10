#include "ChatSignIn.h"

Package *ChatSignIn::process(Json::Value &root){
    try{
        std::string email = root["email"].asString();
        std::string passwd = root["passwd"].asString();
        LOG_DEBUG("注册账户 email[%s] passwd[%s]\n",email.data(),passwd.data());
    }catch (std::exception &e) {
        LOG_ERROR("json error:%s", e.what());
    }
    return nullptr;
}