#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 1
#define BUFFER_SIZE 1024

void client(const char *ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        perror("ERROR invalid address");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(1);
    }
    printf("Connected to server %s:%d\n", ip, port);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLHUP;

    char buffer[BUFFER_SIZE];
    while (1)
    {
        int ret = poll(fds, 2, 500); // Add timeout of 500 milliseconds
        if (ret < 0)
        {
            perror("ERROR polling");
            break;
        }
        else if (ret == 0)
        {
            // Timeout expired and no events to report
            continue;
        }

        if (fds[0].revents & POLLIN)
        {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE - 1, stdin);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                perror("ERROR writing to socket");
                break;
            }
        }


        if (fds[1].revents & POLLIN)
        {
            memset(buffer, 0, BUFFER_SIZE);
            if (read(sockfd, buffer, BUFFER_SIZE - 1) < 0)
            {
                perror("ERROR reading from socket");
                break;
            }
            printf("Server sent: %s", buffer);
        }

        if (fds[1].revents & POLLHUP)
        {
            printf("Connection closed from server side\n");
            close(sockfd);
            break;
        }
    }

    close(sockfd);
}


void server(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("ERROR setting socket options");
        exit(0);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    if (listen(sockfd, 5) < 0)
    {
        perror("ERROR on listening");
        exit(1);
    }
    printf("Server is listening on port %d\n", port);

    while (1)
    {
        int clientfd = accept(sockfd, NULL, NULL);
        if (clientfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }
        printf("Client connected!\n");

        struct pollfd fds[2];
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        fds[1].fd = clientfd;
        fds[1].events = POLLIN;

        char buffer[BUFFER_SIZE];
        while (1)
        {
            if (poll(fds, 2, -1) < 0)
            {
                perror("ERROR polling");
                exit(1);
            }

            if (fds[0].revents & POLLIN)
            {
                memset(buffer, 0, BUFFER_SIZE);
                fgets(buffer, BUFFER_SIZE - 1, stdin);
                if (write(clientfd, buffer, strlen(buffer)) < 0)
                {
                    perror("ERROR writing to socket");
                    exit(1);
                }
            }

            if (fds[1].revents & POLLIN)
            {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t n = read(clientfd, buffer, BUFFER_SIZE - 1);
                if (n < 0)
                {
                    perror("ERROR reading from socket");
                    exit(1);
                }
                else if (n == 0)
                {
                    printf("Client disconnected\n");
                    close(clientfd);
                    break;
                }
                printf("Client sent: %s", buffer);
            }

            if (fds[1].revents & POLLHUP)
            {
                printf("client disconnected\n");
                close(clientfd);
                break;
            }

            if (poll(fds, 2, 0) == 0)
            { // check if timeout expired and no events to report
                if (clientfd < 0)
                { // if the socket is closed, the client has disconnected
                    printf("client disconnected\n");
                    break;
                }
            }
        }
    }

    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s [-c IP PORT|-s PORT]\n", argv[0]);
        exit(1);
    }
    int is_client = 0;
    const char *ip;
    int port;

    if (strcmp(argv[1], "-c") == 0)
    {
        is_client = 1;
        ip = argv[2];
        port = atoi(argv[3]);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        is_client = 0;
        port = atoi(argv[2]);
    }
    else
    {
        printf("Usage: %s [-c IP PORT|-s PORT]\n", argv[0]);
        exit(1);
    }

    if (is_client)
    {
        client(ip, port);
    }
    else
    {
        server(port);
    }

    return 0;
}
