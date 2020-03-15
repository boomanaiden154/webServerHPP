#include <iostream>
#include "server.hpp"

std::string testing(HTTPHeader thing1)
{
    std::cout << "testing the thing" << std::endl;
    HTTPHeader thing(false);
    thing.setProtocol("HTTP/1.1");
    thing.setStatusCode("200");
    thing.setStatus("OK");
    thing.headers["Server"] = "custom/0.0.1";
    thing.headers["Content-Type"] = "text/html";
    thing.headers["Content-Length"] = "8";
    thing.headers["Connection"] = "keep-alive";
    thing.body = "<b>b</b>";
    std::cout << thing.getHeaderString() << std::endl;
    return thing.getHeaderString();
}

int main()
{
    webServer server;
    server.routes["/"] = testing;
    server.initalize();
}
