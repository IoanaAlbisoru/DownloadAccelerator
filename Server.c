#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<dirent.h>
#include "netio.h"
#include "ex3.h"

#define SERVER_PORT 6000
#define MAXBUF 1024
#define MAXCMD 500
#define ROOT "/home/oana/Desktop/proiect" //directorul de start al cautarii

#if MAXCMD > MAXBUF
# error "MAXCMD prea mare"
#endif

int ret;

char *errorcodes[] = {
    "1 Succes\n",
    "2 La revedere\n",
    "3 Eroare la citire\n",
    "4 Eroare la crearea fisierului\n",
    "5 Eroare la scriere\n",
    "6 EOF prematur\n",
    "7 Linia este prea lunga\n",
    "8 Comanda necunoscuta\n",
    "9 Fisier inexistent\n"
};

static inline void reply(int sockfd, int code){
    (void)write(sockfd, errorcodes[code], strlen(errorcodes[code]));
}


int find_file(char *file_name, char *root){
	DIR *src;
	struct dirent *sdir;
	struct stat st;
	char filepath[100];
	
	if((src = opendir(root)) == NULL){
		printf("Eroare la deschiderea directorului.\n");
		exit(1);
	}

	while((sdir = readdir(src)) != NULL){
		if(strcmp(sdir->d_name, ".") && strcmp(sdir->d_name, "..")){
			strcpy(filepath, root);
			strcat(filepath, "/");
			strcat(filepath, sdir->d_name);
			printf("filepath: %s\n", filepath);

			if(lstat(filepath, &st) < 0){
				printf("Eroare la lstat in server.\n");
				exit(2);
			}

			if(S_ISREG(st.st_mode)){
				printf("e fisier\n");
				printf("d_name: --%s--\n", sdir->d_name);
				printf("file_name: --%s--\n", file_name);
				if(strncmp(sdir->d_name, file_name,strlen(sdir->d_name))==0){
					ret = 1;
					
				}
			}
			else if(S_ISDIR(st.st_mode)){
				find_file(file_name, filepath);
			}
		}
	}

	closedir(src);
	
	return ret;
}

void ex3_proto(int connfd) {
    int ret, n, fd, nread;
    char buf[MAXBUF];
    char *file_name = NULL;
    char *cmd;
    
    do{
        printf("Inainte de readline\n");
        ret = readline(connfd, buf, MAXBUF);
        //printf("%s\n", buf);
        cmd = buf;
        n = strspn(cmd, " \t\r\n");
        
        if(strlen(cmd) == n)
            continue;
        cmd += n;
        n = strcspn(cmd, " \t\r\n");
        //printf("%d %s\n", n, cmd);
        if(strncmp(cmd, "quit", n) == 0){
            return;
        }
        if(strncmp(cmd, "search", n) == 0){
            cmd += n + 1;
            file_name = (char *)malloc(strlen(cmd) + 1);
            strcpy(file_name, cmd);
            printf("%s\n", file_name);
            if(!file_name){
                continue;
            }
            if(find_file(file_name, ROOT))
                reply(connfd, EX3_SUCCESS);
            else
                reply(connfd, EX3_INFILE);
            return;
        }
        
        if(strncmp(cmd, "get", n) == 0){
            if((fd = open(file_name, O_RDONLY)) == -1){
                printf("Eroare la deschiderea fisierului %s \n", file_name);
                exit(1);
            }
            while((nread = read(fd, (void *)buf, MAXBUF)) > 0){
                stream_write(connfd, (void *)buf, nread);
            }
            if(nread < 0) {
                printf("Client: Eroare la citire din fisier \n");
                exit(1);
            }
            shutdown(connfd, SHUT_WR);
            close(fd);
            exit(0);
        }
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
        printf("Sunt in server si urmeaza sa fac fork();\n");
        pid = fork();
        switch(pid){
            case -1: printf("Eroare la fork \n"); exit(1);
            case 0: ex3_proto(sockfd); exit(0);
            default: close(connfd);
        }
    }
    exit(0);
}
