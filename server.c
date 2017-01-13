/*************************************************************************
  > File Name: server.c
  > Author: VOID_133
  > Mail: ################### 
  > Created Time: Wed 21 Dec 2016 04:03:25 PM HKT
 ************************************************************************/
#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<string.h>
#include<sys/errno.h>

#define MAX_BUF_SIZE 1000

void usage(char *proc_name) {
    printf("%s [port]\n", proc_name);
    exit(1);
}

void setnonblock(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_flag | O_NONBLOCK);
    return ;
}

int main(int argc, char** argv) {
    int sockfd, clifd;

    //sockaddr_in is used to describe internet(IPV4) socket address
    struct sockaddr_in server_in_addr;
    char buf[MAX_BUF_SIZE];
    int ret = 0;
    int port = 8080;

    if(argv[1] == NULL || argc < 2) {
        usage(argv[0]);
    }
    port = atoi(argv[1]);


    //create the socket flide
    //AF_INET is the address family for internet, SOCK_STREAM means TCP connections
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("socket()");
        exit(-1);
    }
    //Set all bytes to zero
    bzero(&server_in_addr, sizeof(server_in_addr));
    server_in_addr.sin_family = AF_INET;

    //inet_aton convert string based address to binary data
    //Do not use inet_addr, for more info see man page
    ret = inet_aton("0.0.0.0", &server_in_addr.sin_addr);

    //You can also choose the code below to let socket
    //listen to all interface
    //code: server_in_addr.sin_addr.s_addr =  htonl(INADDR_ANY);

    //Convert unsigned short to on-wire data
    server_in_addr.sin_port = htons(port);
    if(ret < 0) {
        perror("inet_aton()");
        exit(-1);
    } 

    //Make the socket resusable
    int enable = 1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, (size_t)sizeof(int));    
    if(ret < 0) {
        perror("setsockopt()");
    }
    //Let socket bind to the server address and port
    ret = bind(sockfd, (const struct sockaddr *)&server_in_addr, sizeof(struct sockaddr_in));
    if(ret < 0) {
        perror("bind()");
        exit(-1);
    }

    //Listen for incoming connections
    ret = listen(sockfd, 50);
    if(ret < 0) {
        perror("listen()");
        exit(-1);
    }
    // Initialize thread_list variable
    fd_set execption_fd, read_fd, write_fd;
    FD_ZERO(&read_fd);
    FD_ZERO(&execption_fd);
    FD_ZERO(&write_fd);
    // Reuse the socket ASAP

    // Don't know why set this will make select continuosly return 0
    // Now set &tval = NULL See select() below
    struct timeval tval;
    tval.tv_sec = 10;
    tval.tv_usec = 0;
    int tot = 0;
    while(1) {
        clifd = accept(sockfd, NULL, NULL);
        if (clifd < 0) {
            perror("accept()");
            exit(-1);
        }

        FD_SET(clifd, &read_fd);
        FD_SET(clifd, &execption_fd);
        ret = select(clifd + 1, &read_fd, NULL, &execption_fd, NULL);
        if(ret < 0) {
            perror("select()");
            exit(-1);
        }
        if(!ret) {
            fprintf(stderr, "data timeout\n");
        }
        if(ret > 0) {
            tot++;
            if(FD_ISSET(clifd, &read_fd)) {
                // Data available
                memset(buf, 0, sizeof(buf));
                while((ret = recv(clifd, buf, MAX_BUF_SIZE, 0)) && ret != EOF) {
                    if(ret < 0) {
                        perror("read()");
                        break;
                    }
                    ret = send(clifd, buf, MAX_BUF_SIZE, 0);
                    if(ret == EOF) {
                        break;
                    }
                    if(ret < 0) {
                        perror("write()");
                        break;
                    }
                }
            }
        }
        close(clifd);
    }
    close(sockfd);
    return 0;
}

