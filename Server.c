#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include "netio.h"
#include "ex3.h"

#define SERVER_PORT 6000
#define BUFFSIZE 1024
#define MAXCMD 500

#if MAXCMD > BUFFSIZE
# error "MAXCMD prea mare"
#endif

char * errorcodes[] = {
    "00 OK \r\n",
    "01 Da-i drumul \r\n",
    "02 La revedere \r\n",
    "03 Eroare la citire din retea \r\n",
    "04 Linia este prea lunga \r\n",
    "05 Nu am putut crea fisierul \r\n",
    "06 Nu am putut scrie fisierul \r\n",
    "07 Conexiunea s-a terminat prematur \r\n",
    "08 Numele fisierului nu e dat \r\n",
    "09 Comanda necunscuta \r\n"
};

static inline void reply(int connfd, int code){
    (void) write(connfd, errorcodes[code], strlen(errorcodes[code]));
}

void getfile(int connfd, char *file_name, char * buf){
    int fd, nread;
    
    if((fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 00644)) <0){
     reply(connfd, EX3_FILECREA);
     return;
    }
    reply(connfd, EX3_GOAHEAD);
    
    while((nread = stream_read(connfd, (void *)buf, 1024)) >= 0){
        if(write(fd, (void *)buf, nread) == -1){
            reply(connfd, EX3_FILEWRERR);
            return;
        }
    }
    close(fd);
    if(nread < 0)
        reply(connfd, EX3_READERR);
    else
        reply(connfd, EX3_SUCCESS);
    return;
}

void ex3_proto(int connfd) {
    int ret, n;
    char buf[BUFFSIZE];
    char *file_name = NULL;
    char *cmd;
    
    do{
        ret = readline(connfd, buf, MAXCMD);
        if(ret != EX3_SUCCESS){
            reply(connfd, ret);
            return;
        }
        
        cmd = buf;
        n = strspn(cmd, " \t\r\n");
        if(strlen(cmd) == n)
            continue;
        cmd += n;
        n = strcspn(cmd, " \t\r\n");
        if(strncmp(cmd, "quit", n) == 0){
            reply(connfd, EX3_BYE);
            return;
        }
        if(strncmp(cmd, "filename", n) == 0){
            cmd += n + 1;
            file_name = (char *)malloc(strlen(cmd) + 1);
            strcpy(file_name, cmd);
            reply(connfd, EX3_SUCCESS);
            continue;
        }
        if(strncmp(cmd, "data", n) == 0){
            if(!file_name){
                reply(connfd, EX3_FNAMNOTSET);
                continue;
            }
            getfile(connfd, file_name, buf);
            return;
        }
        reply(connfd, EX3_INVCMD);
    }while(1);
}

int main(void){
    int sockfd, connfd;
    struct sockaddr_in local_addr, remote_addr;
    socklen_t rlen;
    pid_t pid;
    
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        printf("Eroare la crearea socket-ului \n");
        exit(1);
    }
    set_addr(&local_addr, NULL, INADDR_ANY, SERVER_PORT);
    
    if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1){
        printf("Eroare la bind \n");
        exit(1);
    }
    
    if(listen(sockfd, 5) == 1){
        printf("Eroare la listen \n");
        exit(1);
    }
    
    rlen = sizeof(remote_addr);
    
    while(1){
        connfd = accept(sockfd, (struct sockaddr *)&remote_addr, &rlen);
        if(connfd < 0){
            printf("Eroare la accept \n");
            exit(1);
        }
        pid = fork();
        switch(pid){
            case -1: printf("Eroare la fork \n"); exit(1);
            case 0: close(sockfd); ex3_proto(connfd); exit(0);
            default: close(connfd);
        }
    }
    exit(0);
}
