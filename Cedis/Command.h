#ifndef __COMMAND_H
#define __COMMAND_H

#include <string>
#include <vector>

class command
{
public:
    inline command() {}

    inline command(std::string arg){
        _args.push_back(std::move(arg));
    }

    // 重载 << 将 Type类型，转化为 string 后存入 _args
	template<typename Type>
	inline command& operator<<(const Type arg){
		_args.push_back(std::to_string(arg));
		return *this;
	}

    // 重载 () 将 Type类型，转化为 string 后存入 _args
	template<typename Type>
	inline command& operator()(const Type arg){
		_args.push_back(std::to_string(arg));
		return *this;
	}

    // command ---》 vector<string>&
    inline operator const std::vector<std::string>& () {
        return _args;
    }

    // 用于 debug
    inline std::string toDebugString() {
        std::string ret = "[redis args: ( ";
        bool first = true;
        // 遍历 _args
        for(std::vector<std::string>::iterator iterator = _args.begin(); iterator != _args.end(); ++iterator) {
            // 如果不是第一个，开头添加 ，
            if(!first) ret += ", ";
            first = false;
            ret += "'" + *iterator + "'";
        }
        ret += " )]";
        return ret;
    }

private:
    std::vector<std::string> _args;
};

// 显示具体化
template<>
inline command& command::operator<<(const char* arg){
    _args.push_back(arg);
    return *this;
}

template<>
inline command& command::operator()(const char* arg){
    _args.push_back(arg);
    return *this;
}

template<>
inline command& command::operator<<(std::string arg){
    _args.push_back(std::move(arg));
    return *this;
}

template<>
inline command& command::operator()(std::string arg){
	_args.push_back(std::move(arg));
    return *this;
}

#endif