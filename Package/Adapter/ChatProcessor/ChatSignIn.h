#ifndef CHAT_SIGNIN
#define CHAT_SIGNIN

#include "../Process.h"

class ChatSignIn:public Process{
public:
    ChatSignIn(){

    }
    ~ChatSignIn(){
        
    }

    Package *process(Json::Value &root) override;
};

#endif // !CHAT_MESSAGE