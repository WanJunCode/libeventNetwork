#include "ChatAdapter.h"

ChatAdapter::ChatAdapter(){

}

ChatAdapter::~ChatAdapter(){

}

void ChatAdapter::process(Package *package){
    package->debug();
}