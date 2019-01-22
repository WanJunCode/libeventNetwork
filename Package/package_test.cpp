// #include "ChatPackage.h"
// #include "MultipleProtocol.h"
// #include <stdio.h>
// #include <string.h>
// #include <string>

// using namespace std;

// //字节流转换为十六进制字符串的另一种实现方式
// int byte2hex(const void *sSrc, int nSrcLen, char *sDest, int destLen) {
//     if (nSrcLen * 2 > destLen)
//         return 0;

//     char szTmp[3] = { 0x00 };
//     char *ptr = (char *)sSrc;
//     for (int i = 0; i < nSrcLen; i++) {
//         // string printf
//         sprintf(szTmp, "%02X", (unsigned char)ptr[i]);
//         memcpy(&sDest[i * 2], szTmp, 2);
//     }
//     return nSrcLen * 2;
// }

// // 字符串转换成　16进制的字符串
// std::string byteTohex(const void *sSrc, int nSrcLen) {
//     // 获得　16 进制的长度
//     int len = nSrcLen * 2 + 1;
//     // 智能指针，防止　new 的　char 数组内存泄露
//     std::unique_ptr<char> dst(new char[len]);
//     // unique_ptr::get() 获得唯一指针内部原始指针
//     memset(dst.get(),0x00,len);
//     // sSrc => dst
//     if (byte2hex(sSrc, nSrcLen, dst.get(), len)) {
//         // char[] 隐式转换为　std::string
//         return dst.get();
//     }
//     return "";
// }

// int main(int argc, char const *argv[])
// {
//     // char str[] = argv[1];
//     ChatPackage pkg(ChatPackage::CRYPT_UNKNOW,ChatPackage::DATA_STRING,(void *)argv[1],strlen(argv[1]));
//     string data;
//     // data.append((char *)pkg.getRawData(),pkg.getRawDataLength());
//     printf("rawdata [%s]\n",byteTohex(pkg.getRawData(),pkg.getRawDataLength()).c_str());
//     printf("data [%s]\n",byteTohex(pkg.data(),pkg.length()).c_str());

//     ChatProtocol protocol;
//     if(nullptr != protocol.getOnePackage((BYTE*)pkg.getRawData(),pkg.getRawDataLength())){
//         printf("get package\n");
//     }else{
//         printf("no package\n");
//     }

//     printf("data [%s]\n",data.append((char*)pkg.data(),pkg.length()).c_str());
//     return 0;
// }