// FINAL VERSION I TRY!!!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_CLIENTS 1
#define BUFFER_SIZE 1024
#define SIZE 104857600
#define PIPE "/tmp/pipe"

void generate_file() {
    const char* filename = "file.bin";
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_written = 0;
    srand((unsigned int)time(NULL));
    while (bytes_written < SIZE) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            buffer[i] = (char)rand();
        }
        size_t chunk_size = 1024;
        if (bytes_written + 1024 > SIZE) {
            chunk_size = SIZE - bytes_written;
        }
        fwrite(buffer, 1, chunk_size, file);
        bytes_written += chunk_size;
    }
    fclose(file);
    printf("file created, %d bytes the file %s\n", SIZE, filename);
}

unsigned char calculate_file_checksum(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Failed to open file");
        exit(1);
    }

    unsigned char checksum = 0;
    char buffer;
    while (fread(&buffer, sizeof(char), 1, file) == 1)
    {
        checksum += buffer;
    }

    fclose(file);
    return checksum;
}

void client_ipv4_tcp(int port, const char* ip)
{
    fflush(stdout);
    int sock_addr = socket(AF_INET, SOCK_STREAM,0);
    if(sock_addr < 0){
        perror("eror socket ");
        exit(1);
    }    

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =inet_addr(ip);
    server_addr.sin_port = htons(port);

    printf("port: %d\n", server_addr.sin_port);

    if(connect(sock_addr, (struct  sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("eror connect ");
        exit(1);   
    }

    generate_file();
    char buf[BUFFER_SIZE];
    FILE *f = fopen("transfer.bin", "rb");
    char * filename = "transfer.bin";
    int sent;
    while((sent = fread(buf, 1 , BUFFER_SIZE, f )) > 0 ) 
    {
        if(send(sock_addr, buf, sent, 0) < 0){
            perror("eror send file ");
            exit(1);
        }
    }
    printf("sent the file\n");
    fclose(f);
    close(sock_addr);
}

void server_ipv4_tcp(int port, int quiet)
{
    printf("in server_ipv4_tcp\n");
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("error socket ");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("error bind");
        exit(1);
    }

    if(listen(server_sock, 5) < 0)
    {
        perror("error listen");
        exit(1);
    }

    printf("waiting for connection\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connect_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (connect_sock < 0)
    {
        perror("error accept");
        exit(1);
    }   
    
    char buf[BUFFER_SIZE];
    int sfile = SIZE;
    FILE * f = fopen("newfile_tcp4", "wb");
    int rec;
    clock_t time = clock();
    printf("starting sending file\n");

    while(sfile){
        rec = recv(connect_sock, buf, BUFFER_SIZE, 0);
        while(rec <= 0){
            if(SIZE == sfile) time = clock();
            else break;
        }
        fwrite(buf, rec, 1 , f);
        sfile -= rec;
        memset(buf, 0 , BUFFER_SIZE);
    } 
    time = clock() - time;   
    printf("time send file: %f\n", ((double)time / CLOCKS_PER_SEC) * 1000);
    if(quiet) calculate_file_checksum("newfile_tcp4");
    fclose(f);
    close(connect_sock);
    
}


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

void client_perf(char* argv[])
{
    int port = atoi(argv[3]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("err sock");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port);

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))< 0)
    {
        perror("err connect");
        exit(1);
    }

    if(strcmp(argv[5],"ipv4") == 0)
    {
        if(strcmp(argv[6], "tcp") == 0) 
        {
            printf("sending ipv4 tcp to server\n");
            send(sockfd, "ipv4 tcp", strlen("ipv4 tcp"), 0);
            client_ipv4_tcp(port, argv[2]);
        //else client_ipv4_udp(port, argv[2]);
        }
    }
}

void server_per(char* argv[], int quiet)
{
    printf("in server_per!!!\n");
    int port = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // int enable = 1;
    // if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    // {
    //     perror("ERROR setting socket options");
    //     exit(0);
    // }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
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
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int client_sock = accept(sockfd, (struct sockaddr*) & client_addr, &client_addr_size);
    if(client_sock == -1)
    {
        perror("err sock accept");
        exit(1);
    }

    char buff[1024];
    int bytes = recv(client_sock, &buff, sizeof(buff), 0);
    if(bytes == -1){
        perror("err recv");
        exit(1);
    }

    buff[bytes] = '\0';
    printf("trying to connect ipv4 tcp\n");
    printf("buff --- %s\n", buff);


    if(strcmp(buff, "ipv4 tcp") == 0)
    {
        server_ipv4_tcp(port, quiet);
        return;

    }
}

int main(int argc, char *argv[])
{
    int per = 0, quiet = 0;
    if (argc < 3)
    {
        printf("Usage: %s [-c IP PORT|-s PORT]\n", argv[0]);
        exit(1);
    }
    int is_client = 0;
    const char *ip;
    int port;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0) per = 1;
        if (strcmp(argv[i], "-q") == 0) quiet = 1;
    }
    

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
        if(per == 0)
        {
            client(ip, port);
        }
        client_perf(argv);
    }
    else
    {
        printf("in main i need server perf\n");
        if(per == 0)
        {
            server(atoi(argv[2]));
        }
        printf("should be go here\n");
        server_per(argv, quiet);
    }

    return 0;
}
