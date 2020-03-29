#include "server.hpp"
#include <tuple>
#include <openssl/sha.h>

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
    static std::string SHA1Hash(std::string input)
    {
        std::string toReturn;
        toReturn.resize(20); //SHA1 returns a 160bit value, 20 characters encoded as 8 bits each
        SHA1((unsigned char*)input.c_str(), input.size(), (unsigned char*)toReturn.c_str());
        return toReturn;
    }

    static std::string base64encode(std::string input)
    {
        char encodingTable[] = {'A','B','C','D','E','F','G','H',
            'I','J','K','L','M','N','O','P',
            'Q','R','S','T','U','V','W','X',
            'Y','Z','a','b','c','d','e','f',
            'g','h','i','j','k','l','m','n',
            'o','p','q','r','s','t','u','v',
            'w','x','y','z','0','1','2','3',
            '4','5','6','7','8','9','+','/'};
        
        size_t outputLength = 4 * ((input.size() + 2) / 3);
        std::string toReturn;
        toReturn.resize(outputLength);

        for(auto [i,j] = std::tuple {0, 0}; i < input.size(); i += 3, j += 4)
        {
            uint32_t octetA = i < input.size() ? (unsigned char)input[i] : 0;
            uint32_t octetB = i + 1 < input.size() ? (unsigned char)input[i + 1] : 0;
            uint32_t octetC = i + 2 < input.size() ? (unsigned char)input[i + 2] : 0;

            uint32_t triple = (octetA << 16) + (octetB << 8) + octetC;

            toReturn[j] = encodingTable[(triple >> 18) & 0b00111111];
            toReturn[j + 1] = encodingTable[(triple >> 12) & 0b00111111];
            toReturn[j + 2] = encodingTable[(triple >> 6) & 0b00111111];
            toReturn[j + 3] = encodingTable[triple & 0b00111111];
        }

        //pad the output with equal signs
        int modTable[] = {0, 2, 1};
        for(int i = 0; i < modTable[input.size() % 3]; i++)
        {
            toReturn[outputLength - 1 - i] = '=';
        }

        return toReturn;
    }

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
