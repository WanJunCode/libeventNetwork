#include "EchoAdapter.h"

EchoAdapter::EchoAdapter(){

}

EchoAdapter::~EchoAdapter(){
    
}

Package *EchoAdapter::adapter(Package *package){
    package->debug();
    return nullptr;
} 