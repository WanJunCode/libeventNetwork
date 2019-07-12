#include "ChatMessage.h"

// 注册业务流程有两种：
// １．接受客户端的申请
// ２．设置账号密码
Package *ChatMessage::process(Json::Value &root){
    try{

    }catch (std::exception &e) {
        LOG_ERROR("json error:%s", e.what());
    }
    return nullptr;
}