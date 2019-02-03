#include "MainServer.h"
#include "MainConfig.h"
#include "Signal/PosixSignal.h"

int main(int argc, char const *argv[]){
    PosixSignal::register_signals();
    MainConfig config(CONFIG_PATH);
    MainServer server;
    LOG_DEBUG("***********************wanjun server start***********************\n");
    server.serve();
    LOG_DEBUG("************************wanjun server end************************\n");
    return 0;
}