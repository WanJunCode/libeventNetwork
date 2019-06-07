#ifndef WANJUN_PACKAGE
#define WANJUN_PACKAGE

#include "../main/logcpp.h"
#include "../System/config.h"

#include <stdint.h>
#include <string>
#include <memory>

typedef unsigned char BYTE;

class Package{
protected:
    std::string     rawData;
    std::string     factoryCode;
    size_t          rawDataLength;
public:
    explicit Package():
        rawDataLength(0){
    }
    explicit Package(void *buf,size_t buf_len):
        rawDataLength(buf_len){
        // std::string.append( ... )
        rawData.append((char*)buf,buf_len);
    }

    virtual ~Package(){
        
    }

    inline size_t getRawDataLength() const{
        return rawDataLength;
    }
    inline void *getRawData() const{
        return (void *)rawData.c_str();
    }
    inline std::string& getFactoryCode(){
        return factoryCode;
    }
    bool operator()(){
        return (0==rawDataLength);
    }

    virtual void debug(){
        LOG_DEBUG("package debug\n");
    }

    virtual std::string innerData() {
        return "";
    }
};


class ProtocolEventHandler{
public:
    ProtocolEventHandler(){
    }
    virtual ~ProtocolEventHandler(){
    }
};

// Protocol == ProtocolEventHandler
class Protocol{
public:
    explicit Protocol(ProtocolEventHandler *handler = nullptr):
        handler_(handler){
    }
    virtual ~Protocol(){}

public:
    inline void setEventHandler(ProtocolEventHandler *handler = nullptr){
        handler_ = handler;
    }

    inline ProtocolEventHandler *getEventHandler() {
        return handler_;
    }

    virtual bool parseOnePackage(BYTE * package, size_t dataSize, size_t &framePos, size_t &frameSize, size_t &readWant) = 0;

    virtual Package *getOnePackage(BYTE * package, size_t dataSize) = 0;

    // 虚函数有自己的实现，子类可以不覆盖该函数
    virtual void addProtocol(std::shared_ptr<Protocol> protocol) {
        UNUSED(protocol);
    }

    virtual BYTE *serialize(Package *package) {
        return (BYTE *)package->getRawData();
    }
    
    //反序列化数据
    virtual Package * deserialize(Package *package) {
        return package;
    }

protected:
    ProtocolEventHandler *handler_;
};



#endif //  WANJUN_PACKAGE