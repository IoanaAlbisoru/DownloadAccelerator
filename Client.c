#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "netio.h"
#include "ex3.h"

#define SERVER_PORT 6000
#define MAXBUF 1024

int main(int argc, char *argv[]){
    int fd, sockfd;
    char buf[MAXBUF];
    struct sockaddr_in local_addr, remote_addr;
    int nread, ret;
    char *delimitator = " \r\n";
    int ack;
    
    if(argc < 3 || argc > 4){
        printf("%s adresa fisier [nume] \n", argv[0]);
        exit(1);
    }
    
    if((fd = open(argv[2], O_RDONLY)) == -1){
        printf("Eroare la deschiderea fisierului %s \n", argv[1]);
        exit(1);
    }
    
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        printf("Eroare la socket \n");
        exit(1);
    }
    set_addr(&local_addr, NULL, INADDR_ANY, 0);
    
    if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1){
        printf("Eroare la bind \n");
        exit(1);
    }
    set_addr(&remote_addr, argv[1], 0, SERVER_PORT);
    
    if(connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1){
        printf("Eroare la conectare \n");
        exit(1);
    }
    
    snprintf(buf, MAXBUF, "filename %s%s", argv[argc-1], delimitator);
    stream_write(sockfd, (void *)buf, strlen(buf));
    ret = readline(sockfd, buf, MAXBUF);
    if(ret != EX3_SUCCESS){
        printf("Client: Eroare raspuns \n");
        exit(1);
    }
    ack = atoi(buf);
    switch(ack) {
        case EX3_SUCCESS: break;
       default: printf("%s\n", buf); exit(1);
    }
    snprintf(buf, MAXBUF, "data%s", delimitator);
    stream_write(sockfd, (void *)buf, strlen(buf));
    ret = readline(sockfd, buf, MAXBUF);
    
    if(ret != EX3_SUCCESS){
        printf("Client: Eroare raspuns \n");
        exit(1);
    }
    ack = atoi(buf);
    switch(ack){
        case EX3_GOAHEAD: break;
        default: printf("%s \n", buf); exit(1);
    }
    
    while((nread = read(fd, (void *)buf, MAXBUF)) > 0){
        stream_write(sockfd, (void *)buf, nread);
    }
    if(nread < 0) {
        printf("Client: Eroare la citire din fisier \n");
        exit(1);
    }
    shutdown(sockfd, SHUT_WR);
    close(fd);
    readline(sockfd, buf, MAXBUF);
    printf("%s \n", buf);
    exit(0);
}