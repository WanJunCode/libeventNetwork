#ifndef WANJUN_PACKAGE
#define WANJUN_PACKAGE

#include <stdint.h>
#include <string>

class Package{
public:
    explicit Package():
        rawDataLength(0){
    }
    explicit Package(void *buf,size_t buf_len):
        rawDataLength(buf_len){
        // std::string.append( ... )
        rawData.append((char*)buf,buf_len);
    }

    inline size_t getRawDataLength() const{
        return rawDataLength;
    }
    inline void *getRawData() const{
        return (void *)rawData.c_str();
    }
    inline std::string& getFactoryCode(){
        return factoryCode;
    }
    bool operator()(){
        return (0==rawDataLength);
    }

private:
    std::string rawData;
    std::string factoryCode;
    size_t rawDataLength;
};

#endif //  WANJUN_PACKAGE
