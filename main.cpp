#include <iostream>
#include "server.hpp"
#include "signal.h"

webServer server;

std::string testing(struct webServer::request req)
{
    std::cout << "testing the thing" << std::endl;
    return "<b>BIFFY</b>";
}

std::string testing2(HTTPHeader input)
{
    std::cout << input.body << std::endl;
    std::cout << input.body.size() << std::endl;
    return "{\"test\":\"this be some epic JSON bruh\"}";
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
    server.routes["/biffy"] = webServer::processGetRequestString(testing);
    //server.routes["/testing"] = webServer::processGetRequestRaw(testing);
    //server.routes["/biffy2"] = webServer::processPostRequestRaw(testing2);
    server.initalize();
}
