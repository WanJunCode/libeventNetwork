#include "AdapterMap.h"
#include "../../main/logcpp.h"

static struct {
    std::string code;
    std::shared_ptr<Adapter> adapter;
} adapterArry[] = {
    { CHAT_CODE, std::make_shared<ChatAdapter>()},
    { ECHO_CODE, std::make_shared<EchoAdapter>()},
};

AdapterMap::AdapterMap(){
    LOG_DEBUG("adapter map 插入数据\n");
    // adapterArry 数组大小注意
    for (size_t i = 0; i < 2; ++i) {
        adapter_map_.insert(std::pair<std::string, std::shared_ptr<Adapter> >(adapterArry[i].code, adapterArry[i].adapter));
    }
}

std::shared_ptr<Adapter> AdapterMap::queryAdapter(const std::string code){
    LOG_DEBUG("查询 code [%s]\n",code.data());    
    std::map<std::string, std::shared_ptr<Adapter>>::iterator iter = adapter_map_.find(code);
    if (adapter_map_.end() != iter) {
        return iter->second;
    }
    return nullptr;
}
