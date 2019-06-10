#include "ChatAdapter.h"
#include "../ChatPackage.h"
<<<<<<< HEAD
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
=======

#include "ChatProcessor/ChatLogIn.h"
#include "ChatProcessor/ChatMessage.h"

// AdapterMap 通过 adapterArry[] 获取不同的 适配器
static struct {
    std::string cmd;
    std::shared_ptr<Process> process;
} processArry[] = {
    { "login", std::make_shared<ChatLogIn>()},
    { "message", std::make_shared<ChatMessage>()},
};

>>>>>>> 74e761e... 完成 chat message process 后续增加对 chat 业务流程的设计
ChatAdapter::ChatAdapter(){

}

ChatAdapter::~ChatAdapter(){

}

Package *ChatAdapter::adapter(Package *package){
    ChatPackage *chatPkg = dynamic_cast<ChatPackage *>(package);
    if(chatPkg == nullptr){
        LOG_ERROR("dynamic cast down failure\n");
        return nullptr;
    }
    try {
        // 适配器选择合适的 process 进行处理,每个 type 使用一个 process
        Json::Value root;
        if (Json::Reader().parse((char *)chatPkg->data(), root)){
            root["cmd"].asString();

        }else{
            LOG_INFO("json reader parse don't match\n");
        }
    } catch (std::exception &e) {
        LOG_ERROR("json error:%s, payload[%s]", e.what(), std::string((char *)chatPkg->data(), chatPkg->length()).c_str());
    }
    return nullptr;
}