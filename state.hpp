#pragma once

#include "server.hpp"
#include <random>

class stateMiddleware: public middleware
{
    static char getCharacter(int index)
    {
        const char characters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        return characters[index];
    }

    std::random_device randomDevice;
    std::mt19937 mersenneTwister = std::mt19937(randomDevice());
    std::uniform_int_distribution<uint8_t> distribution = std::uniform_int_distribution<uint8_t>(0, 61);
    int idLength;
public: 
    std::map<std::string, std::map<std::string, void*>*> stateInformation;

    stateMiddleware()
    {
        idLength = 32;
    }

    stateMiddleware(int idLength_)
    {
        idLength = idLength_;
    }

    std::string generateRandomString(int length)
    {
        std::string toReturn;
        for(int i = 0; i < length; i++)
        {
            toReturn += getCharacter(distribution(mersenneTwister));
        }
        return toReturn;
    }

    void processRequest(struct webServer::request& req)
    {
        std::map<std::string, std::string> cookies = HTTPHeader::parseCookiesFromHeader(req.header);
        if(cookies.count("stateID") != 0)
        {
            //add information about the state to the request struct
            (*req.data)["state"] = stateInformation[cookies["stateID"]];
            (*req.data)["stateID"] = &cookies["stateID"];
            (*req.data)["stateCookiePresent"] = (void*)true;
        }
        else
        {
            (*req.data)["state"] = new std::map<std::string, void*>();
            (*req.data)["stateCookiePresent"] = (void*)false;
        }
    }

    void processResponse(struct webServer::response& res)
    {
        if((bool)(*res.data)["stateCookiePresent"])
        {
            //state exists, take state from res and put it into the store
            stateInformation[*(std::string*)(*res.data)["stateID"]] = (std::map<std::string, void*>*)(*res.data)["state"];
        }
        else
        {
            //create new state
            std::string stateID = generateRandomString(idLength);
            HTTPHeader::addCookieToHeader("stateID", stateID, res.header);
            stateInformation[stateID] = (std::map<std::string, void*>*)(*res.data)["state"];
        }
    }
};
