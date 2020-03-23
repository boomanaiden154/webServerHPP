#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <functional>
#include <fstream>
#include <sstream>
#include <vector>

#define PORT 8081

class HTTPHeader
{
public:
    std::multimap<std::string, std::string> headers;
    bool isRequest;
    std::string headerField1;
    std::string headerField2;
    std::string headerField3;
    std::string body;
    //parser state, so that if the header is in multiple packets it can still be parsed
    std::string buffer;
    int bufferIndex;
    std::string fieldName;
    std::string fieldValue;
    bool parsingFieldName;
    bool parsingHeader;

    std::string requestType()
    {
        return headerField1;
    }

    std::string path()
    {
        return headerField2;
    }

    std::string protocol()
    {
        if(isRequest)
        {
            return headerField3;
        }
        else
        {
            return headerField1;
        }
    }

    void setProtocol(std::string toSet)
    {
        if(isRequest)
        {
            headerField3 = toSet;
        }
        else
        {
            headerField1 = toSet;
        }
    }

    std::string statusCode()
    {
        return headerField2;
    }

    void setStatusCode(std::string toSet)
    {
        headerField2 = toSet;
    }

    std::string status()
    {
        return headerField3;
    }

    void setStatus(std::string toSet)
    {
        headerField3 = toSet;
    }

    HTTPHeader(bool isRequest_)
    {
        bufferIndex = 0;
        parsingFieldName = true;
        isRequest = isRequest_;
        parsingHeader = true;
    }

    HTTPHeader()
    {
        HTTPHeader(true);
    }

    void setDefaultResponseHeaders()
    {
        setProtocol("HTTP/1.1");
        setStatusCode("200");
        setStatus("OK");
        headers.insert(std::pair<std::string, std::string>("Server","custom/0.0.1"));
    }

    void parse(std::string input)
    {
        if(parsingHeader)
        {
            buffer += input;
            if(bufferIndex == 0)
            {
                while(buffer[bufferIndex] != ' ')
                {
                    headerField1 += buffer[bufferIndex];
                    bufferIndex++;
                }
                bufferIndex++;
                while(buffer[bufferIndex] != ' ')
                {
                    headerField2 += buffer[bufferIndex];
                    bufferIndex++;
                }
                bufferIndex++;
                while(buffer[bufferIndex] != '\r')
                {
                    headerField3 += buffer[bufferIndex];
                    bufferIndex++;
                }
                bufferIndex += 2;
            }
            while(buffer[bufferIndex] != '\r' && buffer[bufferIndex + 1] != '\n')
            {
                if(parsingFieldName)
                {
                    while(buffer[bufferIndex] != ':' && buffer[bufferIndex + 1] != ' ')
                    {
                        fieldName += buffer[bufferIndex];
                        bufferIndex++;
                    }
                    bufferIndex += 2;
                    parsingFieldName = false;
                }
                else
                {
                    while(buffer[bufferIndex] != '\r' && buffer[bufferIndex + 1] != '\n')
                    {
                        fieldValue += buffer[bufferIndex];
                        bufferIndex++;
                    }
                    bufferIndex += 2;
                    headers.insert(std::pair<std::string,std::string>(fieldName,fieldValue));
                    fieldName.clear();
                    fieldValue.clear();
                    parsingFieldName = true;
                }
            }
            bufferIndex += 2; //for final \r\n
            body += buffer.substr(bufferIndex, buffer.size() - bufferIndex);
            parsingHeader = false;
        }
        else
        {
            body += input;
        }
    }

    std::string getHeaderString()
    {
        std::string toReturn;
        toReturn += headerField1 + " " + headerField2 + " " + headerField3 + "\r\n";
        std::map<std::string, std::string>::iterator itr;
        for(itr = headers.begin(); itr != headers.end(); itr++)
        {
            toReturn += itr->first + ": " + itr->second + "\r\n";
        }
        toReturn += "\r\n";
        toReturn += body;
        return toReturn;
    }

    struct cookie
    {
        std::string name;
        std::string value;
    };

    static std::vector<struct cookie> parseCookiesFromHeader(HTTPHeader input)
    {
        std::vector<struct cookie> toReturn;
        if(input.headers.find("Cookie") != input.headers.end())
        {
            int index = 0;
            while(index < input.headers.find("Cookie")->second.size())
            {
                std::string name;
                while(input.headers.find("Cookie")->second[index] != '=')
                {
                    name += input.headers.find("Cookie")->second[index];
                    index++;
                }
                index++;
                std::string value;
                while(input.headers.find("Cookie")->second[index] != ';' && index < input.headers.find("Cookie")->second.size())
                {
                    value += input.headers.find("Cookie")->second[index];
                    index++;
                }
                index++;
                struct cookie newCookie {name, value};
                toReturn.push_back(newCookie);
            }
        }
        return toReturn;
    }

    static void addCookieToHeader(const struct cookie inputCookie, HTTPHeader& input)
    {
        input.headers.insert(std::pair<std::string,std::string>("Set-Cookie", inputCookie.name + "=" + inputCookie.value));
    }
};

//forward declaration for middleware
class middleware;

class webServer
{
public:
    struct request
    {
        HTTPHeader header;
        std::map<std::string, void*>* data;
        int sockfd;
    };

    struct response
    {
        std::map<std::string, void*>* data;
        HTTPHeader header;
    };

    std::map<std::string, std::function<void(const struct request&, struct response&)>> routes;
    int serverSockFD;
    std::vector<middleware*> serverMiddleware;

    struct connection
    {
        int sockfd;
        struct sockaddr address;
        int addressLength;
        std::map<std::string, std::function<void(const struct request&, struct response&)>>* routes;
        std::vector<middleware*>* serverMiddleware;
    };

    static std::function<void(const struct request&, struct response&)> processGetRequestString(std::function<std::string(struct request)>);
    static std::function<void(const struct request&, struct response&)> processGetRequestFile(std::string, std::string);
    static std::function<void(const struct request&, struct response&)> processPostRequestRaw(std::function<std::string(struct request)>, std::string);
    static void* processConnection(void*);
    void initalize();
    void closeServer();
};

class middleware
{
public:
    virtual void processRequest(struct webServer::request&) {};
    virtual void processResponse(struct webServer::response&) {};
};

std::function<void(const struct webServer::request&, struct webServer::response&)> webServer::processGetRequestString(std::function<std::string(struct webServer::request)> toSendFunction)
{
    return [toSendFunction](const struct request& req, struct response& res)
    {
        std::string toSend = toSendFunction(req);

        HTTPHeader headerToSend(false);
        headerToSend.setDefaultResponseHeaders();
        headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(toSend.size())));
        headerToSend.body = toSend;

        res.header = headerToSend;
    };
}

std::function<void(const struct webServer::request&, struct webServer::response&)> webServer::processGetRequestFile(std::string fileName, std::string mimeType)
{
    std::ifstream inputStream(fileName);
    std::stringstream stringStream;
    stringStream << inputStream.rdbuf();
    std::string body = stringStream.str();
    
    HTTPHeader headerToSend(false);
    headerToSend.setDefaultResponseHeaders();
    headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(body.size())));
    headerToSend.body = body;

    return [headerToSend](const struct request& req, struct response& res)
    {
        res.header = headerToSend;
    }; 
}

std::function<void(const struct webServer::request&, struct webServer::response&)> webServer::processPostRequestRaw(std::function<std::string(struct webServer::request)> processFunction, std::string mimeType = "application/json")
{
    return [processFunction, mimeType](const struct request& req, struct response& res)
    {
        std::string toSend = processFunction(req);

        HTTPHeader headerToSend(false);
        headerToSend.setDefaultResponseHeaders();
        headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(toSend.size())));
        headerToSend.body = toSend;

        struct response toReturn;
        toReturn.header = headerToSend;

        return toReturn;
    };
}

void* webServer::processConnection(void* pointer)
{
    char buffer[2048];
    connection* conn = (connection*)pointer;

    int recieved = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
    buffer[recieved] = '\0';
    struct request requestInput;
    HTTPHeader header(true);
    header.parse(std::string(buffer));
    requestInput.header = header;
    std::map<std::string, void*> data;
    requestInput.data = &data;

    if((*conn->routes).count(header.path()))
    {
        //run middleware on request
        for(int i = 0; i < (*conn->serverMiddleware).size(); i++)
        {
            (*conn->serverMiddleware)[i]->processRequest(requestInput);
        }
        struct response toSend;
        toSend.data = &data;
        (*conn->routes)[header.path()](requestInput, toSend);
        //run middleware on output
        for(int i = 0; i < (*conn->serverMiddleware).size(); i++)
        {
            (*conn->serverMiddleware)[i]->processResponse(toSend);
        }
        std::string toSendString = toSend.header.getHeaderString();
        if(toSendString.size() != 0)
        {
            send(conn->sockfd, toSendString.c_str(), toSendString.size(), 0);
        }
    }
    else
    {
        //return 404
    }

    close(conn->sockfd);
    delete(conn);
    pthread_exit(0);
}

void webServer::initalize()
{
    struct sockaddr_in address;
    int port;

    serverSockFD = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(serverSockFD, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
    listen(serverSockFD, 5);

    while(1)
    {
        pthread_t thread;
        struct connection* conn = (struct connection*)malloc(sizeof(struct connection));
        conn->sockfd = accept(serverSockFD, &conn->address, (socklen_t*)&conn->addressLength);
        conn->routes = &routes;
        conn->serverMiddleware = &serverMiddleware;
        if(conn->sockfd <= 0)
        {
            delete(conn);
        }
        else
        {
            pthread_create(&thread, 0, processConnection, (void*)conn);
            pthread_detach(thread);
        }
    }
}

void webServer::closeServer()
{
    close(serverSockFD);
}
