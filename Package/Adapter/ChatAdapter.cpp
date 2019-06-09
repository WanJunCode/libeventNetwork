#include "ChatAdapter.h"
#include "../ChatPackage.h"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
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