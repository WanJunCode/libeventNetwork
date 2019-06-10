#ifndef WJ_PROCESS_H
#define WJ_PROCESS_H

#include "Adapter.h"

class Package;
// 作为业务处理类的基类
class Process{
public:
    Process(){}
    virtual ~Process() {}
    virtual Package *process(Json::Value &root) = 0;
};

#endif // !WJ_PROCESS_H