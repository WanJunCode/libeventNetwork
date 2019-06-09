#include "EchoPackage.h"
#include <arpa/inet.h>

EchoPackage::EchoPackage(void *payload, size_t length)
    :Package(payload,length){
    BYTE *ptr = (BYTE *)payload;
    EchoPackage::ECHO_HEADER_t *header = (EchoPackage::ECHO_HEADER_t *)ptr;
    data_length = ntohs(header->length);
    factoryCode = ECHO_CODE;
}

EchoPackage::~EchoPackage(){

}

// framePos  frameSize  readWant
bool
EchoProtocol::parseOnePackage(BYTE * package, size_t dataSize, size_t &framePos, size_t &frameSize, size_t &readWant){
    UNUSED(dataSize);
    BYTE *ptr=package;
    EchoPackage::ECHO_HEADER_t *header = (EchoPackage::ECHO_HEADER_t *)ptr;
    uint16_t length = ntohs(header->length);
    if(header->identity == 0x7E){
        framePos = 0;
        readWant = 0;
        frameSize = length + sizeof(EchoPackage::ECHO_HEADER_t);
        return true;
    }else{
        return false;
    }
}

Package *
EchoProtocol::getOnePackage(BYTE * package, size_t dataSize){
    size_t framePos,frameSize,readWant = 0;
    if(parseOnePackage(package,dataSize,framePos,frameSize,readWant)){
        if(frameSize<=dataSize && framePos==0 && readWant==0 ){
            return new EchoPackage(package,frameSize);
        }
    }
    return nullptr;
}