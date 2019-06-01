#include "MainServer.h"
#include "MainConfig.h"
#include "../Signal/PosixSignal.h"
#include "../System/config.h"

#include "../Cedis/RedisPool.h"

int main(int argc, char const *argv[]){
    UNUSED(argc);
    UNUSED(argv);
    // 取消信号的处理，方便调试
    // PosixSignal::register_signals();
    MainConfig *config = new MainConfig(CONFIG_PATH);
    MainServer server(config);
    LOG_DEBUG("***********************wanjun server start***********************\n");
    server.serve();
    LOG_DEBUG("************************wanjun server end************************\n");
    delete config;
    return 0;
}

int redis_test(int argc,char const *argv[]){
    // 二段式构造函数
    std::shared_ptr<Redis> redis1;
    {
        std::shared_ptr<RedisPool> pool;
        pool = make_shared<RedisPool>("localhost",6379,"wanjun",20,15);
        pool->init();
        redis1 = pool->grabCedis();
        std::shared_ptr<Redis> redis2 = pool->grabCedis();
        redis2->reuse();
    }
    redis1->reuse();
    return 0;
}