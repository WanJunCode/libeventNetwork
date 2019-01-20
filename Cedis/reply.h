#ifndef __REPLY_H
#define __REPLY_H

#include <string>
#include <vector>

// 声明 hiredis 中的结构体
struct redisReply;
class TCedis;

/**
 * ＠brief reply contain the value of return , it can be useds as int or string / vector;
 */
class reply
{
public:
    reply(redisReply *reply);               // 构造函数
    reply();

    enum class type_t{
        STRING = 1,
        ARRAY = 2,
        INTEGER = 3,
        NIL = 4,
        STATUS = 5,
        ERROR = 6,
    };

    /**
     * @brief Type of reply, other field values are dependent of this
     * @return
     */
    inline type_t type() const { return _type; }
    /**
     * @brief Returns string value if present, otherwise an empty string
     * @return
     */
    inline const std::string& str() const { return _str; }
    /**
     * @brief Returns integer value if present, otherwise 0
     * @return
     */
    inline long long integer() const { return _integer; }
    /**
     * @brief Returns a vector of sub-replies if present, otherwise an empty one
     * @return
     */
    inline const std::vector<reply>& elements() const { return _elements; }

    // reply  ---> string&
    inline operator const std::string&() const { return _str; }
    // reply  ---> long long
    inline operator long long() const { return _integer; }
    // 重载运算符 == ， 【reply == string】比较，如果type 的 str存在，则比较是否和 rvalue 相等
    inline bool operator==(const std::string& rvalue) const{
		if (_type == type_t::STRING || _type == type_t::ERROR || _type == type_t::STATUS){
            return _str == rvalue;
        }
        else{
            return false;
        }
     }
    // 重载运算符 == 判断 reply 的 integer 是否存在且和 rvalue 相等
    inline bool operator==(const long long rvalue) const{
		if (_type == type_t::INTEGER){
            return _integer == rvalue;
        }else{
            return false;
        }
    }

    // 设置断开状态
    void setDisconn(bool state){
        isDis=state;
    }
    // 判断是否断开
    bool isDisconn(){
        return isDis;
    }

private:
    type_t _type;                           // reply 的类型
    std::string _str;                       // 存储 str
    long long _integer;                     // 存储 integer
    std::vector<reply> _elements;           // 存储 elements
    bool isDis;                             // 判断是否断开 默认是false
};

#endif