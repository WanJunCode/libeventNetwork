#include "EchoAdapter.h"

EchoAdapter::EchoAdapter(){

}

EchoAdapter::~EchoAdapter(){
    
}

void EchoAdapter::process(Package *package){
    package->debug();
} 