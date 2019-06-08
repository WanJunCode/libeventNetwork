#ifndef ECHO_ADAPTER
#define ECHO_ADAPTER

#include "Adapter.h"

class Process;

class EchoAdapter : public Adapter{
private:

public:
    EchoAdapter();
    ~EchoAdapter();

    void process(Package *package) override;

};

#endif // !CHAT_ADAPTER