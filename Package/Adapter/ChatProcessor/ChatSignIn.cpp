#include "ChatSignIn.h"
#include "../../../Mysql/MysqlPool.h"
#include <chrono>


Package *ChatSignIn::process(Json::Value &root){
    try{
        std::string email = root["email"].asString();
        std::string passwd = root["passwd"].asString();
        LOG_DEBUG("注册账户 email[%s] passwd[%s]\n",email.data(),passwd.data());

        auto conn = MysqlPool::getInstance()->getMysqlWrapper();
        conn->test();

    }catch (std::exception &e) {
        LOG_ERROR("json error:%s", e.what());
    }
    return nullptr;
}