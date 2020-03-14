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
    std::string buffer;

    void parse(std::string input)
    {

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
