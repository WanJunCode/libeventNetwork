#ifndef WANJUN_MULTIPLE_PROTOCOL
#define WANJUN_MULTIPLE_PROTOCOL

#include "Package.h"
#include <vector>
#include <memory>

class Package;
class Protocol;
class ProtocolEventHandler;
class MultipleProtocol:public Protocol{
public:
    using Protocol_t = std::vector<std::shared_ptr<Protocol> >;
    MultipleProtocol(ProtocolEventHandler* headler=nullptr);
    virtual ~MultipleProtocol();

public:
    virtual bool parseOnePackage(BYTE *package,size_t dataSize,size_t &framePos,size_t &frameSize,size_t &readWant) override;
    virtual Package *getOnePackage(BYTE * package, size_t dataSize) override;    
    virtual void addProtocol(std::shared_ptr<Protocol> protocol) override;

private:
    Protocol_t protocols_;
};

#endif // !WANJUN_MULTIPLE_PROTOCOL