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

int ret;
char entire_filename[100];
int total_size;
int segSize;
int offset;
char *errorcodes[] = {
    "0 Succes\r\n",
    "1 continua\r\n",
    "2 La revedere\r\n",
    "3 Eroare la citire\r\n",
    "4 Eroare la crearea fisierului\r\n",
    "5 Eroare la scriere\r\n",
    "6 EOF prematur\r\n",
    "7 Linia este prea lunga\r\n",
    "8 Comanda necunoscuta\r\n",
    "9 Fisier inexistent\r\n"
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
				//printf("e fisier\n");
				//printf("d_name: --%s--\n", sdir->d_name);
				//printf("file_name: --%s--\n", file_name);
				if(strncmp(sdir->d_name, file_name,strlen(sdir->d_name))==0){
                    strcpy(entire_filename,filepath);
                    total_size = st.st_size;
                    //printf("totalsize=%i\n",total_size);
					return 1;
					
				}
			}
			else if(S_ISDIR(st.st_mode)){
				find_file(file_name, filepath);
			}
		}
	}

	closedir(src);
	
	return 0;
}

void ex3_proto(int connfd) {
    int ret, n, fd, nread;
    char buf[MAXBUF];
    char *file_name = NULL;
    char *cmd;
    char curDir[100];
    
    do{
        printf("Inainte de readline\n");
        ret = readline(connfd, buf, MAXBUF);
        printf("buf = %s\n",buf);
        // printf("Dupa de readline\n");
        // if(ret !=EX3_SUCCESS)
        // return;         
        printf("%i\n", ret);
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
            file_name[strlen(file_name)-1]='\0';
            // printf("%s\n", file_name);
            if(!file_name){
                continue;
            }
            getcwd(curDir, 100);
            strcat(curDir,"/rootDir");
            if(find_file(file_name, curDir))
                reply(connfd, EX3_SUCCESS);
            else
                reply(connfd, EX3_INFILE);
            continue;
        }

        if(strncmp(cmd, "size", n) == 0){
            char a[20];
            snprintf(a, sizeof(a), "%d", total_size);
            strcat(a,"\r\n");
            // printf("a=%s\n",a);
            write(connfd,a,strlen(a));
            continue;
        }
        //primeste segSize
        if(strncmp(cmd, "segment", n) == 0){
            cmd += n + 1;
            char a[20];
            strcpy(a, cmd);
            segSize=atoi(a);
            // printf("segSize=%i\n",segSize);
            continue;
        }
        
        if(strncmp(cmd, "get", n) == 0){
            // printf("Am ajuns in comanda get\n");
            cmd += n + 1;
            char a[20];
            strcpy(a, cmd);
            offset=atoi(a);
            // printf("offset=%i\n",offset);
            if((fd = open(entire_filename , O_RDONLY)) == -1){
                printf("Eroare la deschiderea fisierului %s \n", entire_filename);
                exit(1);
            }
            lseek(fd,offset,SEEK_SET);
            while((nread = read(fd, (void *)buf, segSize)) > 0){
                stream_write(connfd, (void *)buf, nread);
                segSize-=nread;
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
    if(-1 == bind (sockfd , (struct sockaddr*)&local_addr,sizeof(local_addr))){
        printf("Eroare la bind \n");
        exit(1);
    }
    
    if(listen(sockfd, 5) == -1){
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
            case 0:  close(sockfd); 
                     ex3_proto(connfd); 
                     exit(0);
            default: close(connfd);
        }
    }
    exit(0);
}
