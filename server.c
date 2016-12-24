/*************************************************************************
  > File Name: server.c
  > Author: VOID_133
  > A very simple tcp echo server illustrate use of socket
  > Ver1 only accept single connection, no multiple connection allowed
  > Mail: ################### 
  > Created Time: Wed 21 Dec 2016 04:03:25 PM HKT
 ************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<string.h>
#include<sys/errno.h>

#define MAX_BUF_SIZE 1000

void usage(char *proc_name) {
    printf("%s [port]\n", proc_name);
    exit(1);
}

int main(int argc, char** argv) {
    int sockfd, clifd;

    //sockaddr_in is used to describe internet(IPV4) socket address
    struct sockaddr_in server_in_addr;
    char buf[MAX_BUF_SIZE];
    int ret = 0;
    int port = 8080;
    pid_t child_pid;

    if(argv[1] == NULL) {
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
    while(1) {
        //accept will block until a client connect to the server
        clifd = accept(sockfd, NULL, NULL);
        if(clifd < 0) {
            perror("accept()");
            exit(-1);
        }
        //Here we should fork a process to handle the connection
        child_pid = fork();
        if(child_pid < 0) {
            perror("fork()");
            exit(-1);
        }
        //parent process, continue accept connections
        if(child_pid != 0) {
            close(clifd);
            continue;
        }

        printf("Connect fd = %d\n", clifd);
        memset(buf, 0, sizeof(buf));

        //read from the client until client close/or send EOF
        while((ret = read(clifd, buf, (size_t)MAX_BUF_SIZE)) && ret != EOF) {
            if(ret < 0) {
                perror("read()");
                exit(-1);
            }
            //printf("Get data %s\n", buf);

            //write back to the client
            ret = write(clifd, buf, strlen(buf) * sizeof(char));
            if(ret < 0) {
                perror("write()");
                exit(-1);
            }
        }
        ret = close(clifd);
        if(ret) {
            perror("close()");
        }
        exit(0);
    }
    return 0;
}

