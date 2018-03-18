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

char *errorcodes[] = {
    "1 Succes\r\n",
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

int main(int argc, char * argv[]){
    int fd, sockfd[argc-3];
    char buf[MAXBUF];
    struct sockaddr_in local_addr, remote_addr;
    int nread, ret;
    unsigned int serverCount = 0;
    char *delimitator = "\n";
    int sockfdIndex = 0, argvServerIndex = 2;
    
    if(argc < 4) {
        printf("Mod de apel: %s <nume_fisier> <lista_servere> <nr_segmente> \n", argv[0]);
        exit(1);
    }
    
    //numarul de servere date
    for(int i = 2; i < argc - 1; ++i) //porneste dupa nume_fisier pana la nr_segmente, exclusiv
        serverCount++;
    
    printf("%d\n",serverCount);
    //se creaza un socket diferit pentru fiecare conexiune(client-server) in parte
    while(sockfdIndex < serverCount){
        
        if((sockfd[sockfdIndex] = socket(PF_INET, SOCK_STREAM, 0)) == -1){
            printf("Eroare la socket \n");
            exit(1);
        }
        set_addr(&local_addr, NULL, INADDR_ANY, 0);
        
        if(bind(sockfd[sockfdIndex], (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1){
            printf("Eroare la bind \n");
            exit(1);
        }
        printf("%s\n",argv[argvServerIndex]);
        set_addr(&remote_addr, argv[argvServerIndex], 0, SERVER_PORT);
        
        if(connect(sockfd[sockfdIndex], (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1){
            printf("Eroare la conectare \n");
            exit(1);
        }
        printf("Urmeaza sa intreb serverul daca are fisierul.\n");
        //intreb serverul daca are fisierul
        
        //scriu in buffer codul (search) si numele fisierului pe care doresc sa-l caut
        snprintf(buf, MAXBUF, "search %s%s", argv[1], delimitator);
        printf("%s \n", buf);
        //scriu in socket informatia
        int a1;
        a1=stream_write(sockfd[sockfdIndex], (void *)buf,  MAXBUF);
        printf("a1=%i\n",a1);
        printf("s-a intors din stream_write.\n");
            //astept ca serverul sa raspunda comenzii
        ret = readline(sockfd[sockfdIndex], buf, MAXBUF);
        printf("ret: %d %s \n", ret,buf);
        
            //inchide conexiunea daca fisierul nu a fost gasit
        if(ret !=0){
               close(sockfd[sockfdIndex]);
                sockfdIndex++;
                argvServerIndex++;
                continue; //sari la urmatorul server
        }
            
        //sfarsit confirmare fisierul
        
        
        //scriu in buffer codul(get) si numele fisierului pe care il doresc
        snprintf(buf, MAXBUF, "get %s%s", argv[1], delimitator);
        
        //scriu in socket comanda
        printf("Se trimite la server comanda %s",buf);
        stream_write(sockfd[sockfdIndex], (void *)buf, MAXBUF);
        
        //astept raspunsul serverului
        //ret = readline(sockfd[sockfdIndex], buf, MAXBUF);
        //printf("%i,%s",ret,buf);
        // if(ret != EX3_SUCCESS){
        //     printf("Client: Eroare raspuns. \n");
        //     exit(1);
        // }
        //preluarea fisierului
            fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 00644); 
    
            if(fd == -1){
                printf("eroare aici\n");
                //reply(sockfd[sockfdIndex], EX3_FILECREA);
                return;
            }
            
            //citirea fisierului
            printf("Am ajuns si aici\n");
            while((nread = stream_read(sockfd[sockfdIndex], (void *)buf, MAXBUF)) > 0){
                 printf("BUF=%d\n",nread);
                if(write(fd, (void *)buf, nread) == -1){
                    //reply(sockfd[sockfdIndex], EX3_FILEWRERR);
                    return;
                }
            }
            close(fd);
            // if(nread < 0)
            //     reply(sockfd[sockfdIndex], EX3_READERR);
            // else
            //     reply(sockfd[sockfdIndex], EX3_SUCCESS);
        //sfarsitul preluarii
        
        sockfdIndex++;
        argvServerIndex++;
    }
    
    return 0;
    
}
