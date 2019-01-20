#ifndef CHAT_PACKAGE
#define CHAT_PACKAGE

#include "Package.h"

class ChatPackage:public Package{
public:

private:
    typedef struct _chat_header{
        uint8_t flag;
    }__attribute__((packed)) CHAT_HEADER;
};

#endif // !CHAT_PACKAGE