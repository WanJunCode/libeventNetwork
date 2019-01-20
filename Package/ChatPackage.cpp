#include "ChatPackage.h"
#include <arpa/inet.h>
#include "CRC.h"

#define FACTORY_WANJUN_CODE "WanJun"
#define CHAT_LENGTH ( sizeof(ChatPackage::CHAT_HEADER_t) + sizeof(ChatPackage::CHAT_TAIL_t) )

// head =========== data ============ tail
ChatPackage::ChatPackage(void *payload, size_t length)
    :Package(payload,length),
    data_length(length - CHAT_LENGTH){
    head_ = (CHAT_HEADER_t *)payload;
    tail_ = (CHAT_TAIL_t *)((uint8_t *)payload + sizeof(CHAT_HEADER_t) + data_length);
    factoryCode = FACTORY_WANJUN_CODE;
}

ChatPackage::ChatPackage(CRYPT_TYPE crypt, DATA_TYPE type, void *payload, size_t length)
    :Package(),
    data_length(length){
    factoryCode = FACTORY_WANJUN_CODE;

    rawDataLength = data_length + CHAT_LENGTH;
    
    CHAT_HEADER_t header;
    header.identify = 0x7A;
    header.length = htons(rawDataLength);
    header.version = 0x01;
    // enum as 0x01
    header.type = type;
    header.crypt = crypt;
    header.retain = 0x00;

    rawData.append((char *)&header,sizeof(CHAT_HEADER_t));
    head_ = (CHAT_HEADER_t *)rawData.c_str();

    rawData.append((char *)payload,data_length);
    CHAT_TAIL_t tail;

    // CRC 冗余校验
    tail.crc = htons(crc16((unsigned char *)rawData.c_str(), sizeof(CHAT_HEADER_t) + data_length));
    tail.tail = 0x7B;
    rawData.append((char *)&tail,sizeof(CHAT_TAIL_t));
    tail_ = (CHAT_TAIL_t *)(CHAT_HEADER_t *)( (uint8_t *)head_ + sizeof(CHAT_HEADER_t) + data_length );
}