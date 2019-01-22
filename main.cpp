#include "MainServer.h"
#include "MainConfig.h"

#include "Package/Package.h"

int main(int argc, char const *argv[])
{
    Package p;
    if(p()==0){
        printf(" p is null\n");
    }else{
        printf(" p is not null\n");
    }
    MainConfig config(CONFIG_PATH);
    
    LOG_DEBUG("***********************wanjun server start***********************\n");
    MainServer server;
    server.serve();
    LOG_DEBUG("************************wanjun server end************************\n");
    return 0;
}