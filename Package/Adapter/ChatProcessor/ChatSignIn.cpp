#include "ChatSignIn.h"
#include "../../../Mysql/MysqlPool.h"
#include <chrono>
#include "../../ChatPackage.h"

Package *ChatSignIn::process(Json::Value &root){
    try{
        if(root.isMember("id")){
            // 设置密码
            std::string password = root["password"].asString();
            LOG_DEBUG("设置密码 id[%s]:password[%s]\n",root["id"].asCString(),password.data());
        }else{
            // 申请账户
            std::string phone = root["phone"].asString();
            auto id = MysqlPool::getInstance()->getMysqlWrapper()->createAccount(phone.data());
            std::ostringstream oss;
            if(id){
                oss<<"{\"cmd\":\"signin\",\"id\":\""<<id<<"\"}";
            }else{
                oss<<"{\"cmd\":\"signin\",\"id\":null}";
            }
            // 组包并发送
            auto response = new ChatPackage(ChatPackage::CRYPT_UNKNOW,ChatPackage::DATA_STRING,(void *)oss.str().data(),oss.str().length());
            return response;
        }
    }catch (std::exception &e) {
        LOG_ERROR("json error:%s\n", e.what());
    }
    return nullptr;
}