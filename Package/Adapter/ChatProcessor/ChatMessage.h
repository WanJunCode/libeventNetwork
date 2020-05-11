#ifndef CHAT_MESSAGE
#define CHAT_MESSAGE

#include "../Process.h"

class ChatMessage:public Process{
public:
    ChatMessage(){

    }
    ~ChatMessage(){
        
    }

    Package *process(Json::Value &root) override;
};

#endif // !CHAT_MESSAGE