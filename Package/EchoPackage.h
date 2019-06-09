#ifndef ECHO_PACKAGE
#define ECHO_PACKAGE

#include "Package.h"
#include <assert.h>
#include "string"
#include "../main/logcpp.h"

class EchoPackage : public Package{
public:
    typedef struct ECHO_HEADER_t{
        uint8_t     identity;   //0x7E
        uint16_t    length;     //两个字节
    }__attribute__((packed)) ECHO_HEADER_t;

    EchoPackage(void *payload, size_t length);
    ~EchoPackage();

    virtual void debug() override{
        LOG_DEBUG("echo package debug\n");
        uint8_t *tmp_ptr = (uint8_t *)Package::getRawData();
        LOG_DEBUG("Recv RawData : [%s]\n", byteTohex((void *)(tmp_ptr + sizeof(ECHO_HEADER_t)), data_length).c_str());
        std::string innerdata;
        innerdata.append((char *)(tmp_ptr + sizeof(ECHO_HEADER_t)),data_length);
        LOG_DEBUG("inner data [%s]\n",innerdata.c_str());
    }

    virtual std::string innerData() override{
        uint8_t *tmp_ptr = (uint8_t *)Package::getRawData();
        std::string str;
        str.append((char *)(tmp_ptr + sizeof(ECHO_HEADER_t)),data_length);
        return str;
    }
    
private:
    // ECHO_HEADER_t   *header;
    uint16_t        data_length;
};

class EchoProtocol : public Protocol{
public:
    explicit EchoProtocol(ProtocolEventHandler *headler=nullptr):
        Protocol(headler){};
    virtual ~EchoProtocol(){};
public:
    virtual bool parseOnePackage(BYTE * package, size_t dataSize, size_t &framePos, size_t &frameSize, size_t &readWant);
    virtual Package *getOnePackage(BYTE * package, size_t dataSize);
};

#endif // !ECHO_PACKAGE