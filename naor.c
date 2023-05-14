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
    const char* filename = "random.bin";
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

    if(connect(sock_addr, (struct  sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("eror connect ");
        exit(1);    }

    char buf[BUFFER_SIZE];
    int f = open("newfile.bin", O_RDONLY);
    off_t offset = 0;
    int sent;
    while( (sent = sendfile(sock_addr, f, &offset,BUFFER_SIZE ))  > 0 ) {
        
        if(sent == -1){
            perror("eror send file ");
            exit(1);
        }
    }
    close(f);
    close(sock_addr);
}

void server_ipv4_tcp(int port, int quiet)
{
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

    while(sfile > 0 ){
        rec = recv(connect_sock, buf, BUFFER_SIZE, 0);
        if(rec == -1){
            perror("error recive");
            exit(1);
        }
        sfile -= rec;
    } 
    time = clock() - time;   
    printf("time send file: %f\n", (double)time);
    calculate_file_checksum("newfile_tcp4");
    fclose(f);
    close(connect_sock);
    
}

void client_ipv6_tcp(int port,const char* ip)
{

    fflush(stdout);
    int sock_addr6 = socket(AF_INET6, SOCK_STREAM, 0);
    if(sock_addr6 < 0){
        perror("eror socket ");
        exit(1);

    }    
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    if(connect(sock_addr6, (struct  sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("eror connect ");
        exit(1);    }

    char buf[BUFFER_SIZE];
    int f = open("newfile.bin", O_RDONLY);
    off_t offset = 0;
    int sent;
    while( (sent = sendfile(sock_addr6, f, &offset,BUFFER_SIZE ))  > 0 ) {
        
        if(sent == -1){
            perror("eror send file ");
            exit(1);
        }
    }
    close(f);
    close(sock_addr6);

    
}
void server_ipv6_tcp(int port, int quiet)
{
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("error socket ");
        exit(1);
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port);

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

    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connect_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (connect_sock < 0)
    {
        perror("error accept");
        exit(1);
    }   
    
    char buf[BUFFER_SIZE];
    int sfile = SIZE;
    FILE * f = fopen("newfile_tcp6", "wb");
    int rec;
    clock_t time = clock();

    while(sfile > 0 ){
        rec = recv(connect_sock, buf, BUFFER_SIZE, 0);
        if(rec == -1){
            perror("error recive");
            exit(1);
        }
        sfile -= rec;
    } 
    time = clock() - time;   
    printf("time send file: %f\n", (double)time);
    calculate_file_checksum("newfile_tcp6");
    fclose(f);
    close(connect_sock);
}

void client_ipv4_udp(int port, const char* ip)
{
    int sock_addr = socket(AF_INET, SOCK_DGRAM ,0);
    if(sock_addr < 0){
        perror("eror socket ");
        exit(1);

    }    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =inet_addr(ip);
    server_addr.sin_port = htons(port);

    char buf[BUFFER_SIZE];
    FILE * f = fopen("newfile.bin", "rb");
    int sent, len;
    while( (len = fread(buf, 1, BUFFER_SIZE, f ))  > 0 ) {
        sent = sendto(sock_addr, buf, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent < 0)
        {
            perror("error sentdo");
            exit(1);
        }
    }
    fclose(f);
    close(sock_addr);
}

void server_ipv4_udp(int port, int quiet)
{
    int sock_addr = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_addr < 0){
        perror("error socket");
        exit(1);
    }

    printf("waiting for connection\n");

    char buf[BUFFER_SIZE];
    FILE * f = fopen("mewfile_udp4", "wb");
    int rec, len;
    socklen_t client_addr_len;
    struct sockaddr_in client_addr;
    while(1)
    {
        len = recvfrom(sock_addr, buf, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (len < 0)
        {
            perror("error recvfrom ");
            exit(1);
        }
        fwrite(buf, 1 , len , f);
        if ( len < BUFFER_SIZE) break;
    }

    calculate_file_checksum("newfile_udp4");
    fclose(f);
    close(sock_addr);
}

void client_ipv6_udp(int port, const char* ip)
{
    int sock_addr = socket(AF_INET, SOCK_DGRAM ,0);
    if(sock_addr < 0){
        perror("eror socket ");
        exit(1);

    }    
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(port);

    char buf[BUFFER_SIZE];
    FILE * f = fopen("newfile.bin", "rb");
    int sent, len;
    while( (len = fread(buf, 1, BUFFER_SIZE, f ))  > 0 ) {
        sent = sendto(sock_addr, buf, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent < 0)
        {
            perror("error sentdo");
            exit(1);
        }
    }
    fclose(f);
    close(sock_addr);
}

void server_ipv6_udp(int port, int quiet)
{
    int sock_addr = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock_addr < 0){
        perror("error socket");
        exit(1);
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port);

    if(bind(sock_addr, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("error bind");
        exit(1);
    }

    printf("waiting for connection\n");


    char buf[BUFFER_SIZE];
    FILE * f = fopen("mewfile_udp6", "wb");
    int rec, len;
    socklen_t client_addr_len;
    struct sockaddr_in6 client_addr;
    while(1)
    {
        len = recvfrom(sock_addr, buf, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (len < 0)
        {
            perror("error recvfrom ");
            exit(1);
        }
        fwrite(buf, 1 , len , f);
        if ( len < BUFFER_SIZE) break;
    }

    calculate_file_checksum("newfile_udp6");
    fclose(f);
    close(sock_addr);
}

void server_udsdgram()
{
    printf("TODO!!!\n"); // TODO !!!!!
}

void client_udsdgram()
{
    printf("TODO!!!\n"); // TODO !!!!!
}

void server_udsstream()
{
    printf("TODO!!!\n");   // TODO!!!!
}

void client_udsstream()
{
    printf("TODO!!!\n");   // TODO!!!!
}

void server_mmap(char *filename, int quiet)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd == -1)
    {
        perror("open error");
        exit(1);
    }

    if(ftruncate(fd, SIZE) < 0)
    {
        perror("error resize");
        exit(1);
    }

    void * mmmap = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
    if (mmmap == MAP_FAILED)
    {
        perror("mmap err");
        exit(1);
    }
    int new_fd = open("mmp.bin", O_WRONLY | O_CREAT, 0666);
    clock_t time = clock();
    if (new_fd == -1)
    {
        perror("err");
        exit(1);
    }
    if(write(new_fd, mmmap, SIZE) == -1)
    {
        perror("err");
        exit(1);
    }
    clock_t etime = clock() - time;
    double total_time = (double)(etime - time) / CLOCKS_PER_SEC;
    printf("mmap time %f\n", total_time * 1000);
    if(quiet) calculate_file_checksum("mmp.bin");
    if(munmap(mmmap, SIZE) == -1)
    {
        perror("munmap err");
        exit(1);
    }
    close(fd);
    close(new_fd);

}

void client_mmap(char *filename)
{
    int fd = open("mmap", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("err");
        exit(1);
    }

    char *mmmap = mmap(NULL,SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ( mmmap == MAP_FAILED)
    {
        perror("mmap err");
    }

    char buff[BUFFER_SIZE];
    FILE * f = fopen(filename, "rb");
    if ( !f)
    {
        perror("open err");
        exit(1);
    }

    while(fread(buff, 1 , BUFFER_SIZE, f) > 0) memcpy(mmmap, buff, BUFFER_SIZE);
    
    if (munmap(mmmap, SIZE) == -1)
    {
        perror("munmap err");
        exit(1);
    }
    close(fd);
}

void client_pipe(char *filename)
{
    FILE * f;
    f = fopen(filename, "r");
    if (!f)
    {
        perror("fopen error");
        exit(1);
    }

    int fd = open(PIPE, O_WRONLY);
    if (fd == -1)
    {
        perror("open error");
        exit(1);
    }
    char buff[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = fread(buff, sizeof(char), sizeof(buff), f)) > 0)
    {
        if (write(fd, buff, bytes) != bytes)
        {
            perror("write error");
            exit(1);
        }
    }
    close(fd);
    fclose(f);
}

void server_pipe(char *filename, int quiet)
{
    int fd;
    FILE * f;
    char buff[BUFFER_SIZE];
    int bytes;

    mkfifo(PIPE, 0666);

    int pipe_fd = open(PIPE, O_RDONLY);
    if (pipe_fd == -1)
    {
        perror("error open");
        exit(1);
    }

    clock_t time = clock();
    while((bytes = read(pipe_fd, buff, BUFFER_SIZE)) > 0)
    {
        fwrite(buff, sizeof(char), bytes, f);
    }

    time = clock() - time;
    double total_time = (double)(time) / CLOCKS_PER_SEC;
    printf("pipe time %f\n", total_time * 1000);
    if(quiet) calculate_file_checksum("mmp.bin");
    close(pipe_fd);
    fclose(f);
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
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
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
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLHUP;
    fds[1].revents = 0;

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



void client_perf(char* argv[])
{
    int port = atoi(argv[2]);
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

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))< 0)
    {
        perror("err connect");
        exit(1);
    }

    if(strcmp(argv[5],"ipv4") == 0)
    {
        if(strcmp(argv[6], "tcp") == 0) client_ipv4_tcp(port,argv[2]);
        else client_ipv4_udp(port, argv[2]);
    }
    else if(strcmp(argv[5], "ipv6")==0)
    {
        if(strcmp(argv[6], "tcp") == 0) client_ipv6_tcp(port,argv[2]);
        else client_ipv6_udp(port, argv[2]);
    }

    else if(strcmp(argv[5], "mmap") == 0) client_mmap(argv[6]);
    else if(strcmp(argv[5], "pipe") == 0) client_pipe(argv[6]);

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

void server_perf(char* argv[], int quiet)
{
    int port = atoi(argv[3]);
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
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[3]));

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
    char buff[BUFFER_SIZE];
    int bytes = recv(client_sock, &buff, sizeof(buff), 0);
    if(bytes == -1){
        perror("err recv");
        exit(1);
    }

    buff[bytes] = '\0';

    if(strcmp(buff,"ipv4 tcp") == 0)
    {
        server_ipv4_tcp(port + 1, quiet);
        return;
    }

    else if(strcmp(buff,"ipv4 udp") == 0)
    {
        server_ipv4_udp(port + 1, quiet);
        return;
    }

    else if(strcmp(buff,"ipv6 tcp") == 0)
    {
        server_ipv6_tcp(port + 1, quiet);
        return;
    }

    else if(strcmp(buff,"ipv6 udp") == 0)
    {
        server_ipv6_udp(port + 1, quiet);
        return;
    }
    
    else if(strcmp(buff, "uds stream") == 0)
    {
        server_udsstream(quiet);
        return;
    }

    else if(strcmp(buff, "uds dgram") == 0)
    {
        server_udsdgram(quiet);
        return;
    }

    else if(strcmp(buff, "mmap") == 0)
    {
        char *filename = "mmap";
        server_mmap(filename, quiet);
        return;
    }

    else if(strcmp(buff, "pipe") == 0)
    {
        char *filename = "pipe";
        server_mmap(filename, quiet);
        return;
    }
  
}

int main(int argc, char *argv[])
{
    int perf = 0, quiet = 0;

    if (argc < 3 || argc > 7)
    {
        printf("Usage: %s [-c IP PORT|-s PORT]\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0) perf = 1;
        else if (strcmp(argv[i], "-q") == 0) quiet = 1;
    }

    int is_client = 0;
    const char *ip;
    int port;
    char * type;
    char * param;

    if (strcmp(argv[1], "-c") == 0)
    {
        is_client = 1;
        ip = argv[2];
        port = atoi(argv[3]);

        if(!perf)
        {
            client(ip, port);
            return 0;
        }
        client_perf(argv);
        return 0;

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
        if( !perf) server(port);
        else server_perf(argv, quiet);
    }

    return 0;
}
