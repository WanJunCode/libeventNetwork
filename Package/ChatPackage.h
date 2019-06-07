#ifndef CHAT_PACKAGE
#define CHAT_PACKAGE

#include "Package.h"
#include <assert.h>
#include "../main/logcpp.h"
#include "string"

class ChatPackage:public Package{
public:
    typedef struct CHAT_HEADER_t{
        uint8_t     identify;
        uint16_t    length;
        uint8_t     version;
        uint8_t     type;
        uint8_t     crypt;
        uint8_t     retain;
    }__attribute__((packed)) CHAT_HEADER_t;

    typedef struct _CHAT_TAIL_t{
        uint16_t    crc;
        uint8_t     tail;
    }__attribute__((packed)) CHAT_TAIL_t;

    typedef enum{
        CRYPT_UNKNOW = 0x00,
        CRYPT_AES128 
    } CRYPT_TYPE;

    typedef enum{
        DATA_BINARY = 0x00,
        DATA_STRING = 0x01,
        DATA_FILE = 0x02
    } DATA_TYPE;

public:
    ChatPackage(void *payload, size_t length);
    ChatPackage(CRYPT_TYPE crypt, DATA_TYPE TYPE, void *payload, size_t length);
    ~ChatPackage(){};

    virtual void debug() override{
        LOG_DEBUG("chat package debug\n");
        std::string msg;
        msg.append((char *)data(),length());
        LOG_DEBUG("chat message : [%s]\n",msg.data());
    }
public:
    // 判断 payload 指针指向的内存区域是否是一个完整的数据包
    static bool isOnePackage(void *payload, size_t length,size_t& frame,size_t& want);
public:
    inline DATA_TYPE type() const{
        assert(head_);
        return (DATA_TYPE)head_->type;
    }
    inline CRYPT_TYPE crypt() const{
        assert(head_);
        return (CRYPT_TYPE)head_->crypt;
    }
    // string.c_str() 返回　const char * ，是不可以被修改的
    inline void *data(){
        return (void *)((char *)rawData.c_str() + sizeof(CHAT_HEADER_t));
    }
    inline size_t length() {
        return data_length;
    }

private:
    CHAT_HEADER_t *head_;
    CHAT_TAIL_t *tail_;
    uint16_t data_length;
};

class ChatProtocol:public Protocol{
public:
    explicit ChatProtocol(ProtocolEventHandler *headler=nullptr):
        Protocol(headler){};
    virtual ~ChatProtocol(){};

public:
    virtual bool parseOnePackage(BYTE * package, size_t dataSize, size_t &framePos, size_t &frameSize, size_t &readWant);
    virtual Package *getOnePackage(BYTE * package, size_t dataSize);
};

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

#endif // !CHAT_PACKAGE