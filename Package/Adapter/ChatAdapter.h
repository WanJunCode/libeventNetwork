#ifndef CHAT_ADAPTER
#define CHAT_ADAPTER

#include "Adapter.h"

class Process;

class ChatAdapter : public Adapter{
private:

public:
    ChatAdapter();
    ~ChatAdapter();

    void process(Package *package) override;

};

#endif // !CHAT_ADAPTER