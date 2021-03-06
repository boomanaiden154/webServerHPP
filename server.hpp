#pragma once

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
#include <any>

#define PORT 8081
#define BUFFERSIZE 8192

class HTTPHeader
{
public:
    std::multimap<std::string, std::string> headers;
    bool isRequest;
    std::string headerField1;
    std::string headerField2;
    std::string headerField3;
    std::string body;
    bool isEmpty;
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
        isEmpty = false;
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
        if(!isEmpty)
        {
            if(headerField1.size() != 0 && headerField2.size() != 0 && headerField3.size() != 0)
            {
                    toReturn += headerField1 + " " + headerField2 + " " + headerField3 + "\r\n";
            }
            std::map<std::string, std::string>::iterator itr;
            for(itr = headers.begin(); itr != headers.end(); itr++)
            {
                toReturn += itr->first + ": " + itr->second + "\r\n";
            }
            if(toReturn.size() != 0)
            {
                    toReturn += "\r\n";
            }
            toReturn += body;
        }
        return toReturn;
    }

    static std::map<std::string, std::string> parseCookiesFromHeader(HTTPHeader& input)
    {
        std::map<std::string, std::string> toReturn;
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
                toReturn[name] = value;
            }
        }
        return toReturn;
    }

    static void addCookieToHeader(std::string name, std::string value, HTTPHeader& input)
    {
        input.headers.insert(std::pair<std::string,std::string>("Set-Cookie", name + "=" + value));
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
        std::map<std::string, std::any>& data;
        int sockfd;

        request(std::map<std::string, std::any>& data_): data(data_) {}
    };

    struct response
    {
        std::map<std::string, std::any>& data;
        HTTPHeader header;

        response(std::map<std::string, std::any>& data_): data(data_) {}
    };

    std::map<std::string, std::function<void(struct request&, struct response&)>> routes;
    int serverSockFD;
    std::vector<middleware*> serverMiddleware;

    struct connection
    {
        int sockfd;
        struct sockaddr address;
        int addressLength;
        std::map<std::string, std::function<void(struct request&, struct response&)>>* routes;
        std::vector<middleware*>* serverMiddleware;
    };

    static std::function<void(struct request&, struct response&)> processGetRequestString(std::function<std::string(struct request&)>);
    static std::function<void(struct request&, struct response&)> processGetRequestFile(std::string, std::string);
    static std::function<void(struct request&, struct response&)> processPostRequestRaw(std::function<std::string(struct request&)>, std::string);
    static struct request recieveData(std::map<std::string, std::any>&, struct connection*);
    static void sendData(struct response&, int sockfd);
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

std::function<void(struct webServer::request&, struct webServer::response&)> webServer::processGetRequestString(std::function<std::string(struct webServer::request&)> toSendFunction)
{
    return [toSendFunction](struct request& req, struct response& res)
    {
        std::string toSend = toSendFunction(req);

        HTTPHeader headerToSend(false);
        headerToSend.setDefaultResponseHeaders();
        headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(toSend.size())));
        headerToSend.body = toSend;

        res.header = headerToSend;
    };
}

std::function<void(struct webServer::request&, struct webServer::response&)> webServer::processGetRequestFile(std::string fileName, std::string mimeType)
{
    std::ifstream inputStream(fileName);
    std::stringstream stringStream;
    stringStream << inputStream.rdbuf();
    std::string body = stringStream.str();
    
    HTTPHeader headerToSend(false);
    headerToSend.setDefaultResponseHeaders();
    headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(body.size())));
    headerToSend.body = body;

    return [headerToSend](struct request& req, struct response& res)
    {
        res.header = headerToSend;
    }; 
}

std::function<void(struct webServer::request&, struct webServer::response&)> webServer::processPostRequestRaw(std::function<std::string(struct webServer::request&)> processFunction, std::string mimeType = "application/json")
{
    return [processFunction, mimeType](struct request& req, struct response& res)
    {
        std::string toSend = processFunction(req);

        HTTPHeader headerToSend(false);
        headerToSend.setDefaultResponseHeaders();
        headerToSend.headers.insert(std::pair<std::string, std::string>("Content-Length",std::to_string(toSend.size())));
        headerToSend.body = toSend;

        res.header = headerToSend;
    };
}

struct webServer::request webServer::recieveData(std::map<std::string, std::any>& requestResponseData, struct webServer::connection* conn)
{
    char buffer[BUFFERSIZE];
    int recieved = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
    buffer[recieved] = '\0';
    struct request requestToReturn(requestResponseData);
    HTTPHeader header(true);
    header.parse(std::string(buffer));
    requestToReturn.header = header;
    requestToReturn.sockfd = conn->sockfd;
    return requestToReturn;
}

void webServer::sendData(struct webServer::response& dataToSend, int sockfd)
{
    std::string toSendString = dataToSend.header.getHeaderString();
    if(toSendString.size() != 0)
    {
        send(sockfd, toSendString.c_str(), toSendString.size(), 0);
    }
}

void* webServer::processConnection(void* pointer)
{
    char buffer[2048];
    connection* conn = (connection*)pointer;

    std::map<std::string, std::any> requestResponseData;
    struct request requestInput = recieveData(requestResponseData, conn);

    if((*conn->routes).count(requestInput.header.path()))
    {
        //run middleware on request
        for(int i = 0; i < (*conn->serverMiddleware).size(); i++)
        {
            (*conn->serverMiddleware)[i]->processRequest(requestInput);
        }
        struct response toSend(requestResponseData);
        (*conn->routes)[requestInput.header.path()](requestInput, toSend);
        //run middleware on output
        for(int i = 0; i < (*conn->serverMiddleware).size(); i++)
        {
            (*conn->serverMiddleware)[i]->processResponse(toSend);
        }
        sendData(toSend, conn->sockfd);
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
