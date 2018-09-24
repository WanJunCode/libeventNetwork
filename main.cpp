#include <stdio.h>
#include <string.h>
#include "MainServer.h"

int main(int argc, char const *argv[])
{
    printf("服务器          // wanjun //   开启 ...\n");
    MainServer server(12345);
    server.serve();
    return 0;
}