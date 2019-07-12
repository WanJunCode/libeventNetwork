#include "ChatAdapter.h"
#include "../ChatPackage.h"

#include "Process.h"
#include "ChatProcessor/ChatMessage.h"
#include "ChatProcessor/ChatSignIn.h"

// AdapterMap 通过 adapterArry[] 获取不同的 适配器
static struct {
    std::string cmd;
    std::shared_ptr<Process> process;
} processArry[] = {
    { "signin"  ,   std::make_shared<ChatSignIn>()},
    { "message" ,   std::make_shared<ChatMessage>()},
};

ChatAdapter::ChatAdapter(){
    for (size_t i = 0; i < ARRAY_SIZE(processArry); ++i) {
        processMap.insert(std::pair<std::string, std::shared_ptr<Process> >(processArry[i].cmd, processArry[i].process));
    }
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
            auto proc = processMap.find(root["cmd"].asString());
            if(proc!=processMap.end()){
                auto pkg = proc->second->process(root);
                // 下行报文
                return pkg;
            }
        }else{
            LOG_INFO("json reader parse don't match\n");
        }
    } catch (std::exception &e) {
        LOG_ERROR("json error:%s, payload[%s]", e.what(), std::string((char *)chatPkg->data(), chatPkg->length()).c_str());
    }
    return nullptr;
}