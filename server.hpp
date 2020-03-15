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

#define PORT 8081

class HTTPHeader
{
public:
    std::map<std::string, std::string> headers;
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
    }

    void parse(std::string input)
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
                headers[fieldName] = fieldValue;
                fieldName.clear();
                fieldValue.clear();
                parsingFieldName = true;
            }
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
};

class webServer
{
public:
    std::map<std::string, std::function<std::string(HTTPHeader)>> routes;

    struct connection
    {
        int sockfd;
        struct sockaddr address;
        int addressLength;
        std::map<std::string, std::function<std::string(HTTPHeader)>>* routes;
    };

    static std::function<std::string(HTTPHeader)> processGetRequestRaw(std::function<std::string()> toSendFunction)
    {
        return [&toSendFunction](HTTPHeader header){
            std::string toSend = toSendFunction();

            HTTPHeader headerToSend(false);
            headerToSend.setProtocol("HTTP/1.1");
            headerToSend.setStatusCode("200");
            headerToSend.setStatus("OK");
            headerToSend.headers["Server"] = "custom/0.0.1";
            headerToSend.headers["Content-Type"] = "text/html";
            headerToSend.headers["Content-Length"] = std::to_string(toSend.size());
            headerToSend.headers["Connection"] = "keep-alive";
            headerToSend.body = toSend;

            return headerToSend.getHeaderString();
        };
    }

    static void* processConnection(void* pointer)
    {
        char buffer[2048];
        connection* conn = (connection*)pointer;

        int recieved = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
        buffer[recieved] = '\0';
        HTTPHeader header(true);
        header.parse(std::string(buffer));

        if((*conn->routes).count(header.path()))
        {
            std::string toSend = (*conn->routes)[header.path()](header);
            send(conn->sockfd, toSend.c_str(), toSend.size(), 0);
        }

        close(conn->sockfd);
        delete(conn);
        pthread_exit(0);
    }

    void initalize()
    {
        int sockfd;
        struct sockaddr_in address;
        int port;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        bind(sockfd, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
        listen(sockfd, 5);

        while(1)
        {
            pthread_t thread;
            struct connection* conn = (struct connection*)malloc(sizeof(struct connection));
            conn->sockfd = accept(sockfd, &conn->address, (socklen_t*)&conn->addressLength);
            conn->routes = &routes;
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
};
