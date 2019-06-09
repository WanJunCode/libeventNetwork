#ifndef __CF_ADAPETER_MAPPING_H
#define __CF_ADAPETER_MAPPING_H

#include "../../base/Mutex.h"
#include "../../base/noncopyable.h"
#include "../ChatPackage.h"
#include "ChatAdapter.h"
#include "EchoAdapter.h"
#include <jsoncpp/json/json.h>
#include <map>

class AdapterMap : public noncopyable{
public:
    static AdapterMap& getInstance() {
        static AdapterMap instance;
        return instance;
    }
public:
    std::shared_ptr<Adapter> queryAdapter(const std::string code);

protected:
    AdapterMap();

private:
    std::map< std::string, std::shared_ptr<Adapter> > adapter_map_;
};



#endif
