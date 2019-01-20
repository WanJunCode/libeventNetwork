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
    // short == uint16_t  2个字节
    header.length = htons(rawDataLength);
    header.version = 0x01;
    // enum as 0x01
    header.type = type;
    header.crypt = crypt;
    header.retain = 0x00;
    // 添加　head
    rawData.append((char *)&header,sizeof(CHAT_HEADER_t));
    head_ = (CHAT_HEADER_t *)rawData.c_str();
    // 添加　data
    rawData.append((char *)payload,data_length);
    CHAT_TAIL_t tail;

    // CRC 冗余校验
    tail.crc = htons(crc16((unsigned char *)rawData.c_str(), sizeof(CHAT_HEADER_t) + data_length));
    tail.tail = 0x7B;
    // 添加　tail
    rawData.append( (char *)&tail, sizeof(CHAT_TAIL_t) );
    tail_ = (CHAT_TAIL_t *)(CHAT_HEADER_t *)( (uint8_t *)head_ + sizeof(CHAT_HEADER_t) + data_length );
}

// 判断是否是一个数据包
// 1. 获得　数据包　head　length
// 2. 获得　head flag ,  length 长度是否符合
// 3. 获得的数据长度　是否　大于　data length；　　获取的数据长度不足，　保存已获得的数据长度 frame，记录剩余需要的　数据长度　want;
// 4. 判断　 tail flag , 检验　crc校验，是否有数据错误
// static
bool ChatPackage::isOnePackage(void *payload, size_t length, size_t& frame, size_t& want){
    CHAT_HEADER_t *header = (CHAT_HEADER_t *)payload;
    int datasize = ntohs(header->length);
    if( header->identify==0x7A && datasize>CHAT_LENGTH){
        if(length>=datasize){
            CHAT_TAIL_t *tailer = (CHAT_TAIL_t *)( (uint8_t*)header + (datasize - sizeof(CHAT_TAIL_t)) ); 
            if(tailer->tail==0x7B && tailer->crc==crc16((unsigned char *)header,(datasize-sizeof(CHAT_TAIL_t)))){
                frame = datasize;
                want = 0;
                return true;
            }
        }else{
            frame = datasize;
            want = datasize - length;
            return true;
        }
    }    
    return false;
}