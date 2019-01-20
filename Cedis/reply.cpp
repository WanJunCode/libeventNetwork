#include <hiredis/hiredis.h>
#include "reply.h"

reply::reply(redisReply *c_reply):
    _type(type_t::ERROR),
    _integer(0),
    isDis(false)
{
    // 将 redisReply *c->type (int) ---->  type_t (enum)
    _type = static_cast<type_t>(c_reply->type);
    switch (_type) {
	    case type_t::ERROR:
	    case type_t::STRING:
	    case type_t::STATUS:
            _str = std::string(c_reply->str, c_reply->len);
            break;
	    case type_t::INTEGER:
            _integer = c_reply->integer;
            break;
	    case type_t::ARRAY:
            for (size_t i=0; i < c_reply->elements; ++i) {
                _elements.push_back(reply(c_reply->element[i]));
            }
            break;
        default:
            break;
    }
}

//构造函数
reply::reply():
_type(type_t::ERROR),
_integer(0),
isDis(false){
}
