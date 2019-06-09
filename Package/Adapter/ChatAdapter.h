#ifndef CHAT_ADAPTER
#define CHAT_ADAPTER

#include "Adapter.h"

// 线程安全是因为没有对变量进行修改
class ChatAdapter : public Adapter{
private:
    std::map<std::string,std::shared_ptr<Process> > processMap;
public:
    ChatAdapter();
    ~ChatAdapter();

    Package *adapter(Package *package) override;
};

#endif // !CHAT_ADAPTER