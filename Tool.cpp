#include "Tool.h"

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>

// 将 format 和 变参 转化为 std::string
std::string vform(const char* format, va_list args) {
    size_t size = 1024;
    char* buffer = new char[size];

    while (1) {
        va_list args_copy;
        // 将 args 复制到 args_copy
        va_copy(args_copy, args);

        int n = vsnprintf(buffer, size, format, args_copy);//function error,replace it

        va_end(args_copy);

        // If that worked, return a string.
        if ((n > -1) && (static_cast<size_t>(n) < size)) {
            // char 指针 装换为 std::string
            std::string s(buffer);
            delete [] buffer;
            return s;
        }

        // Else try again with more space.
        size = (n > -1) ?
               n + 1 :   // ISO/IEC 9899:1999
               size * 2; // twice the old size

        delete [] buffer;
        buffer = new char[size];
    }
}

	// 获得绝对路径中 最后的 文件名
const char* StripFileName(const char *full_name) {
	const char *pos = full_name + strlen(full_name);
	// full_name ------/----/--- pos
	while (pos != full_name) {
		-- pos;
		if (*pos == '/') {
			++ pos;
			break;
		}
	}
	return pos;
}

//std::string.c_str()
char *textFileRead(const char* filename){
    FILE *pf = fopen(filename,"r");
    if(pf!=NULL){
        fseek(pf,0,SEEK_END);

        long lsize = ftell(pf);

        rewind(pf);
        char *ptext = (char *)malloc(lsize + 1);
        if(NULL!=ptext){
            if(0<fread(ptext,sizeof(char),lsize,pf)){
                ptext[lsize] = '\0';
            }
        }
        fclose(pf);
        return ptext;
    }
    return NULL;
}