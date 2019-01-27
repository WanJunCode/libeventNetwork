#ifndef WANJUN_PROCESSOR
#define WANJUN_PROCESSOR
#include "Package.h"

class ProcessorEventHandler{
public:
    virtual ~ProcessorEventHandler(){};

public:
    
protected:
    ProcessorEventHandler(){};

};

class Processor{
public:
    Processor(ProcessorEventHandler *handle)
        :eventHandler(handle){};
    virtual ~Processor(){};

public:
    // 基类非　virtual 函数
    bool process(Package *in,Package *&out,Protocol *protocol,void *connection){
        return handler(in,out,protocol,connection);
    }

private:
    virtual bool handler(Package *in,Package *&out,Protocol *protocol,void *connection) = 0;

private:
    ProcessorEventHandler *eventHandler;
};

#endif