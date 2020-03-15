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

#define PORT 8081

class HTTPHeader
{
public:
    std::map<std::string, std::string> headers;
    std::string requestType;
    std::string path;
    std::string protocol;
    //parser state, so that if the header is in multiple packets it can still be parsed
    std::string buffer;
    int bufferIndex;
    std::string fieldName;
    std::string fieldValue;
    bool parsingFieldName;

    HTTPHeader()
    {
        bufferIndex = 0;
        parsingFieldName = true;
    }

    void parse(std::string input)
    {
        buffer += input;
        if(bufferIndex == 0)
        {
            while(buffer[bufferIndex] != ' ')
            {
                requestType += buffer[bufferIndex];
                bufferIndex++;
            }
            bufferIndex++;
            while(buffer[bufferIndex] != ' ')
            {
                path += buffer[bufferIndex];
                bufferIndex++;
            }
            bufferIndex++;
            while(buffer[bufferIndex] != '\r')
            {
                protocol += buffer[bufferIndex];
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

    void printData()
    {
        std::cout << requestType << std::endl;
        std::cout << path << std::endl;
        std::cout << protocol << std::endl;
        std::map<std::string,std::string>::iterator itr;
        for(itr = headers.begin(); itr != headers.end(); itr++)
        {
            std::cout << itr->first << std::endl;
            std::cout << itr->second << std::endl;
        }
    }
};

class webServer
{
    struct connection
    {
        int sockfd;
        struct sockaddr address;
        int addressLength;
    };

    static void* processConnection(void* pointer)
    {
        char buffer[2048];
        connection* conn = (connection*)pointer;

        int recieved = recv(conn->sockfd, buffer, sizeof(buffer) - 1, 0);
        std::cout << recieved << std::endl;
        buffer[recieved] = '\0';
        printf("%s\n", buffer);
        HTTPHeader headerThing;
        headerThing.parse(std::string(buffer));
        headerThing.printData();

        close(conn->sockfd);
        delete(conn);
        pthread_exit(0);
    }

public:
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
