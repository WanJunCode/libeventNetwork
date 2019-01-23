#include "RedisPool.h"

int main2(){
    RedisPool pool("127.0.0.1",6379,"",100,5);
    auto conn = pool.grabCedis();
    if(conn->ping()){
        printf("ping ok\n");
    }else{
        printf("ping false\n");
    }
    return 0;
}

int main1(){
    auto conn = Redis::create();
    if(conn->ping()){
        printf("ping ok\n");
    }else{
        printf("ping false\n");
    }
    return 0;
}