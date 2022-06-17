#include "HttpServer.h"
#include <tinymuduo/EventLoop.h>

using namespace tinymuduo;
using namespace tinymuduo::net;
int main()
{
    int numThreads = 3;
    EventLoop loop1;
    HttpServer server(&loop1, InetAddress(8888), "httpserver");
    server.setThreadNum(numThreads);
    server.start();
    loop1.loop();
    return 0;
}