#include "MainServer.h"
#include "MainConfig.h"
#include "../Signal/PosixSignal.h"
#include "../System/config.h"

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