#include <iostream>
#include "server.hpp"
#include "signal.h"

webServer server;

std::string testing(HTTPHeader input)
{
    std::cout << "testing the thing" << std::endl;
    std::cout << input.body << std::endl;
    std::cout << input.body.size() << std::endl;
    return "<b>BIFFY</b>";
}

void testing2(HTTPHeader input)
{
    std::cout << input.body << std::endl;
}

void sigintHandler(int sig_num)
{
    server.closeServer();
    exit(0);
}

int main()
{
    signal(SIGINT, sigintHandler);
    server.routes["/"] = webServer::processGetRequestFile("testing.jpg","image/jpeg");
    server.routes["/biffy"] = webServer::processGetRequestRaw(testing);
    server.routes["/testing"] = webServer::processGetRequestRaw(testing);
    server.routes["/biffy2"] = webServer::processPostRequestRaw(testing);
    server.initalize();
}
