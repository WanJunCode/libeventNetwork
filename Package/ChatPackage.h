#ifndef CHAT_PACKAGE
#define CHAT_PACKAGE

#include "Package.h"
#include <assert.h>
#include "string"
#include "../main/logcpp.h"

class ChatPackage:public Package{
public:
    typedef struct CHAT_HEADER_t{
        uint8_t     identify;           // 标识
        uint16_t    length;             // 数据长度
        uint8_t     version;            // 版本
        uint8_t     type;               // 类型
        uint8_t     crypt;              // 加密类型
        uint8_t     retain;             // 保留
    }__attribute__((packed)) CHAT_HEADER_t;

    typedef struct _CHAT_TAIL_t{
        uint16_t    crc;                // 冗余校验
        uint8_t     tail;               // 数据尾部标识
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
    CHAT_HEADER_t   *head_;
    CHAT_TAIL_t     *tail_;
    uint16_t        data_length;
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

#endif // !CHAT_PACKAGE