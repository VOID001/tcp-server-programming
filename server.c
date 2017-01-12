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
#include<string.h>
#include<sys/errno.h>

#define MAX_BUF_SIZE 1000

typedef struct thread_args {
    int clifd;
} ThreadArgs;


void usage(char *proc_name) {
    printf("%s [port]\n", proc_name);
    exit(1);
}

int nocopy_echo(int connfd, int pipe_rd, int pipe_wr) {
    ssize_t sz;
    sz = splice(connfd, NULL, pipe_wr, NULL, MAX_BUF_SIZE, SPLICE_F_MORE);
    if(sz < 0) {
        perror("splice() read in");
        return sz;
    }
    sz = splice(pipe_rd, NULL, connfd, NULL, MAX_BUF_SIZE, SPLICE_F_MORE);
    if(sz < 0) {
        perror("splice() write out");
        return sz;
    }
    return 0;
}

// Handle connections per thread
void *handle_conn(void *args) {
    // Here we use an advanced no-copy IO to echo back the data
    ThreadArgs *targs = (ThreadArgs *)args;
    //printf("fd = %d\n", targs->clifd);
    //printf("args = %x\n", args);
    int ret = 0;
    int pipefd[2];
    ret = pipe(pipefd);
    if(ret < 0) {
        perror("pipe()");
        return NULL;
    }
    while(1) {
        ret = nocopy_echo(targs->clifd, pipefd[0], pipefd[1]);
        if(ret < 0) {
            perror("nocopy_echo()");
            break;
        }
        if(!ret)
            break;
    }
    close(targs->clifd);
    close(pipefd[0]);
    close(pipefd[1]);
    free(args);
    return NULL;
}

int main(int argc, char** argv) {
    int sockfd, clifd;

    //sockaddr_in is used to describe internet(IPV4) socket address
    struct sockaddr_in server_in_addr;
    int ret = 0;
    int port = 8080;
    ThreadArgs *targs = NULL;

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
    int thread_cnt = 0;
    pthread_t *thread_list = NULL;
    while(1) {
        //accept will block until a client connect to the server
        //Use pthread_create to create thread for handling connections
        clifd = accept(sockfd, NULL, NULL);
        if (clifd < 0) {
            perror("accept()");
            exit(-1);
        }
        printf("Create thread with fd = %d\n", clifd);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        thread_cnt++;
        targs = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        targs->clifd = clifd;
        thread_list = (pthread_t *)realloc((void *)thread_list, thread_cnt * sizeof(pthread_t));
        ret = pthread_create(&thread_list[thread_cnt - 1], &attr, handle_conn, (void *)targs);
        if(ret < 0) {
            perror("pthread_create()");
            exit(-1);
        }
    }
    return 0;
}

