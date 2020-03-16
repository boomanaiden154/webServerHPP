#include <iostream>
#include "server.hpp"

std::string testing(HTTPHeader input)
{
    std::cout << "testing the thing" << std::endl;
    std::cout << input.body << std::endl;
    std::cout << input.body.size() << std::endl;
    return "<b>BIFFY</b>";
}

int main()
{
    webServer server;
    server.routes["/"] = webServer::processGetRequestFile("testing.jpg","image/jpeg");
    server.routes["/biffy"] = webServer::processGetRequestRaw(testing);
    server.initalize();
}
