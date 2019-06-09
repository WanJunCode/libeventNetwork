#include "AdapterMap.h"
#include "../../main/logcpp.h"

#define ARRAY_SIZE(array)    (sizeof(array)/sizeof(*(array)))

// AdapterMap 通过 adapterArry[] 获取不同的 适配器
static struct {
    std::string code;
    std::shared_ptr<Adapter> adapter;
} adapterArry[] = {
    { CHAT_CODE, std::make_shared<ChatAdapter>()},
    { ECHO_CODE, std::make_shared<EchoAdapter>()},
};

AdapterMap::AdapterMap(){
    // adapterArry 数组大小注意
    for (size_t i = 0; i < ARRAY_SIZE(adapterArry); ++i) {
        adapter_map_.insert(std::pair<std::string, std::shared_ptr<Adapter> >(adapterArry[i].code, adapterArry[i].adapter));
    }
}

std::shared_ptr<Adapter> AdapterMap::queryAdapter(const std::string code){
    std::map<std::string, std::shared_ptr<Adapter>>::iterator iter = adapter_map_.find(code);
    if (adapter_map_.end() != iter) {
        return iter->second;
    }
    return nullptr;
}