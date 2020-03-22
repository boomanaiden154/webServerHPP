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

class middlewareTesting: public middleware
{
    void processRequest(struct webServer::request& req)
    {
        std::cout << "processing request" << std::endl;
    }

    void processResponse(struct webServer::response& res)
    {
        std::cout << "processing response" << std::endl;
    }
};

int main()
{
    signal(SIGINT, sigintHandler);
    server.serverMiddleware.push_back(new middlewareTesting);
    server.routes["/"] = webServer::processGetRequestFile("testing.jpg","image/jpeg");
    server.routes["/biffy"] = webServer::processGetRequestString(testing);
    //server.routes["/testing"] = webServer::processGetRequestRaw(testing);
    //server.routes["/biffy2"] = webServer::processPostRequestRaw(testing2);
    server.initalize();
}
