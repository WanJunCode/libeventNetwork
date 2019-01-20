#ifndef WANJUN_PACKAGE
#define WANJUN_PACKAGE

#include <stdint.h>
#include <string>

typedef unsigned char BYTE;

class Package{
public:
    explicit Package():
        rawDataLength(0){
    }
    explicit Package(void *buf,size_t buf_len):
        rawDataLength(buf_len){
        // std::string.append( ... )
        rawData.append((char*)buf,buf_len);
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

protected:
    std::string rawData;
    std::string factoryCode;
    size_t      rawDataLength;
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

    virtual void addProtocol(Protocol *) {}

    virtual bool parseOnePackage(BYTE * package, size_t dataSize, size_t &framePos, size_t &frameSize, size_t &readWant) = 0;

    virtual Package *getOnePackage(BYTE * package, size_t dataSize) = 0;

    virtual BYTE *serialize(Package *package) {
        return (BYTE *)package->getRawData();
    };
    //反序列化数据
    virtual Package * deserialize(Package *package) {
        return package;
    }

protected:
    ProtocolEventHandler *handler_;
};


#endif //  WANJUN_PACKAGE
