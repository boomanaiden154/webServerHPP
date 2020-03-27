#pragma once

#include "server.hpp"
#include "state.hpp"
#include <functional>

class loginMiddleware: public middleware
{
public:
    struct user
    {
        std::string username;
        std::map<std::string, std::any> userData;
    };

    std::map<std::string, struct user> users;

    static bool userLogIn(struct webServer::request& req, std::function<bool(std::string,std::string)> isUserValid, std::string username, std::string password)
    {
        if(isUserValid(username, password))
        {
            std::cout << "authenticated user into system" << std::endl;
            req.data["authenticated"] = true;
            struct user toAdd;
            toAdd.username = username;
            req.data["user"] = toAdd;
            return true;
        }
        else
        {
            return false;
        }
    }

    static bool isAuthenticated(struct webServer::request& req)
    {
        if(req.data.count("authenticated") != 0 && std::any_cast<bool>(req.data["authenticated"]) == true)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void processRequest(struct webServer::request& req)
    {
        if(users.count(std::any_cast<std::string>(req.data["stateID"])) != 0)
        {
            //user is logged in
            req.data["authenticated"] = true;
            //load user from store
            req.data["user"] = users[std::any_cast<std::string>(req.data["stateID"])];
        }
        else
        {
            //user is not logged in
            req.data["authenticated"] = false;
        }
    }

    void processResponse(struct webServer::response& res)
    {
        if(std::any_cast<bool>(res.data["authenticated"]))
        {
            std::cout << "saving user state" << std::endl;
            users[std::any_cast<std::string>(res.data["stateID"])] = std::any_cast<struct user>(res.data["user"]);
        }
    }
};
