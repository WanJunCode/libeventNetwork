#ifndef WANJUN_PROTOCAL
#define WANJUN_PROTOCAL

class ProtocolEventHandler{
public:
    ProtocolEventHandler(){
    }
    virtual ~ProtocolEventHandler(){
    }
};

class Package;
class Protocol{
public:
    Protocol(ProtocolEventHandler *handler = nullptr):
        handler_(handler){
    }
    virtual ~Protocol(){
    }

public:
    void setEventHandler(ProtocolEventHandler *handler = nullptr){
        handler_ = handler;
    }

    ProtocolEventHandler *getEventHandler() {
        return handler_;
    }


private:
    ProtocolEventHandler *handler_;
};

#endif // !WANJUN_PROTOCAL
