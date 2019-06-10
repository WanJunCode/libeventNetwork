#ifndef CHAT_MESSAGE
#define CHAT_MESSAGE

#include "../Process.h"

class ChatMessage:public Process{
public:
    ChatMessage(){

    }
    ~ChatMessage(){
        LOG_DEBUG("chat message destory\n");
    }

    Package *process(Json::Value &root) override;
};

#endif // !CHAT_MESSAGE