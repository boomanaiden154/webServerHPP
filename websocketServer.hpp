#include "server.hpp"

class websocket
{
    std::map<std::string, std::any> data;
    int sockfd;

    void sendMessage()
    {

    };
};

class websocketServer
{
public:
    std::function<void(std::string, websocket&)> messageCallback;

    std::function<void(struct webServer::request&, struct webServer::response&)> websocketRoute()
    {
        return [this](struct webServer::request& req, struct webServer::response& res)
        {
            //recieve HTTP Upgrade request, send back HTTP 101 switching protocols
            std::cout << req.header.getHeaderString() << std::endl;
            res.header.isEmpty = true;
        };
    }
};
