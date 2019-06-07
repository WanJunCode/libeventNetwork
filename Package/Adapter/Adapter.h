// 适配器,作用是根据数据包找到对应的处理器 process

// TConnection 处理数据包的流程如下

// 1.获得完整的数据包
// 2.根据数据包格式获得相应的 adapter,通过 package factoryCode 判断
// 3.通过 adapter 找到正确的 processor 进行业务处理,数据包通过json解析,获得cmd type
// 4.adapter 1:n processor
#ifndef WJ_ADAPTER
#define WJ_ADAPTER
#include "../Package.h"

class Adapter{
public:
    Adapter();
    virtual ~Adapter();

    virtual void process(Package *package){
        package->debug();
    };
};

#endif