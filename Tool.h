#ifndef WANJUN_TOOL_H
#define WANJUN_TOOL_H

#include <stdarg.h>
#include <iostream>
#include <stdio.h>

// 将 format 和 变参 转化为 std::string
std::string vform(const char* format, va_list args);
const char* StripFileName(const char *full_name);

char *textFileRead(const char* filename);

#endif // !WANJUN_TOOL_H