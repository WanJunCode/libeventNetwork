#include "ChatSignIn.h"
#include "../../../Mysql/MysqlPool.h"
#include <chrono>


Package *ChatSignIn::process(Json::Value &root){
    try{
        std::string email = root["email"].asString();
        std::string passwd = root["passwd"].asString();
        LOG_DEBUG("注册账户 email[%s] passwd[%s]\n",email.data(),passwd.data());

        auto pool = MysqlPool::getInstance();
        auto conn = pool->getMysqlWrapper();

    }catch (std::exception &e) {
        LOG_ERROR("json error:%s\n", e.what());
    }
    return nullptr;
}