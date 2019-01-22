#include "MultipleProtocol.h"
#include <algorithm>

MultipleProtocol::MultipleProtocol(ProtocolEventHandler *headler)
    :Protocol(headler){
}

MultipleProtocol::~MultipleProtocol(){
}

bool 
MultipleProtocol::parseOnePackage(BYTE *package,size_t dataSize,size_t &framePos,size_t &frameSize,size_t &readWant){
    for(auto iter : protocols_){
        if(iter->parseOnePackage(package,dataSize,framePos,frameSize,readWant)){
            return true;
        }
    }
    return false;
}

Package *
MultipleProtocol::getOnePackage(BYTE * package, size_t dataSize){
    for(auto iter : protocols_){
        Package *pkg = iter->getOnePackage(package,dataSize);
        if(pkg){
            return pkg;
        }
    }
    return nullptr;
}

void 
MultipleProtocol::addProtocol(std::shared_ptr<Protocol> protocol){
    auto iter = std::find(protocols_.begin(),protocols_.end(),protocol);
    if(iter==protocols_.end())
        protocols_.emplace_back(protocol);
}