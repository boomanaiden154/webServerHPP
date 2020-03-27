#include <iostream>
#include "server.hpp"
#include "signal.h"
#include "state.hpp"
#include "login.hpp"

webServer server;

std::string testing(struct webServer::request& req)
{
    if(req.data["state"].has_value() && std::any_cast<bool>(req.data["state"]) == true)
    {
        std::cout << "LETS FRICKING GO" << std::endl;
    }
    req.data["state"] = true;
    std::cout << "testing the thing" << std::endl;
    if(std::any_cast<bool>(req.data["authenticated"]))
    {
        return "<i>LETS GO YOU ARE NOW LOGGED IN</i>";
    }
    else
    {
        return "<b>BIFFY - YOU ARE NOT LOGGED IN</b>";
    }
}

bool isUserValid(std::string username, std::string password)
{
    return true;
}

std::string testing2(struct webServer::request& req)
{
    loginMiddleware::userLogIn(req, isUserValid, "biffy", "whatthebif");
    return "<b>YOU SHOULD BE LOGGED IN NOW</b>";
}

void sigintHandler(int sig_num)
{
    server.closeServer();
    exit(0);
}

int main()
{
    signal(SIGINT, sigintHandler);
    server.serverMiddleware.push_back(new stateMiddleware());
    server.serverMiddleware.push_back(new loginMiddleware());
    server.routes["/"] = webServer::processGetRequestFile("testing.jpg","image/jpeg");
    server.routes["/biffy"] = webServer::processGetRequestString(testing);
    server.routes["/login"] = webServer::processGetRequestString(testing2);
    server.initalize();
}
