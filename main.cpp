#include "MainServer.h"
#include "MainConfig.h"

int main(int argc, char const *argv[])
{
    MainConfig config(CONFIG_PATH);
    
    LOG_DEBUG("***********************wanjun server start***********************\n");
    MainServer server;
    server.serve();
    LOG_DEBUG("************************wanjun server end************************\n");
    return 0;
}