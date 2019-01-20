#include "TCedisPool.h"

int example(){

    TCedisPool pool("127.0.0.1",6379,"",100,5);

    auto conn = pool.grabCedis();

    if(conn->ping()){
        printf("ping ok\n");
    }else{
        printf("ping false\n");
    }

    return 0;
}
