#include <iostream>
#include "server.hpp"

std::string testing()
{
    std::cout << "testing the thing" << std::endl;
    return "<b>BIFFY</b>";
}

int main()
{
    webServer server;
    server.routes["/"] = webServer::processGetRequestFile("testing.jpg","image/jpeg");
    server.initalize();
}
