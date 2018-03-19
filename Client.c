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
#define MAXFILEDIM 100000000
#define MAXSEG 20

static char FILE_BUF[100000000];

int main(int argc, char * argv[]){
    int fd, sockfd[MAXSEG]; //cate segmente sunt
    char buf[MAXBUF];
    struct sockaddr_in local_addr, remote_addr;
    int nread, ret;
    unsigned int serverCount = 0;
    char *delimitator = "\n";
    int sockfdIndex = 0, argvServerIndex = 2;
    char size[20];
    int nr_segmente = atoi(argv[argc-1]);
    int total_size;
    int offset=0;
    int ok;
    int segmentsize=0;
    int readoffset=0;
    int toread=0;
    
    if(argc < 4) {
        printf("Mod de apel: %s <nume_fisier> <lista_servere> <nr_segmente> \n", argv[0]);
        exit(1);
    }
    
    //numarul de servere date
    for(int i = 2; i < argc - 1; ++i) //porneste dupa nume_fisier pana la nr_segmente, exclusiv
        serverCount++;
    
    printf("%d\n",serverCount);
    //se creaza un socket diferit pentru fiecare conexiune(client-server) in parte
    while(sockfdIndex < nr_segmente){
        
        if((sockfd[sockfdIndex] = socket(PF_INET, SOCK_STREAM, 0)) == -1){
            printf("Eroare la socket \n");
            exit(1);
        }
        set_addr(&local_addr, NULL, INADDR_ANY, 0);
        
        if(bind(sockfd[sockfdIndex], (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1){
            printf("Eroare la bind \n");
            exit(1);
        }
        printf("%s\n",argv[(argvServerIndex-2)%serverCount+2]);//accesez 
        set_addr(&remote_addr, argv[(argvServerIndex-2)%serverCount+2], 0, SERVER_PORT);
        
        if(connect(sockfd[sockfdIndex], (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1){
            printf("Eroare la conectare \n");
            exit(1);
        }
        printf("Urmeaza sa intreb serverul daca are fisierul.\n");
        //intreb serverul daca are fisierul
        
        //scriu in buffer codul (search) si numele fisierului pe care doresc sa-l caut
        snprintf(buf, MAXBUF, "search %s%s", argv[1], delimitator);
        //printf("%s \n", buf);
        //scriu in socket informatia
        int a1;
        a1=stream_write(sockfd[sockfdIndex], (void *)buf,  MAXBUF);
        //printf("a1=%i\n",a1);
        //printf("s-a intors din stream_write.\n");
            //astept ca serverul sa raspunda comenzii
        ret = readline(sockfd[sockfdIndex], buf, MAXBUF);
        //printf("ret: %d %s \n", ret,buf);
        
            //inchide conexiunea daca fisierul nu a fost gasit
        if(ret != 0){
                close(sockfd[sockfdIndex]);
                sockfdIndex++;
                argvServerIndex++;
                continue; //sari la urmatorul server
        }

        if(sockfdIndex == 0) //dimensiunea totala a fisierului o aflam atunci cand gasim primul server care contine fisierul
        {
            snprintf(size, MAXBUF, "size %s%s",argv[1], delimitator);
            //printf("%s \n", size);  
            stream_write(sockfd[sockfdIndex], (void *)size,  MAXBUF);
            ret = readline(sockfd[sockfdIndex], size, 20); 
            total_size = atoi(size);
            //printf("size=%s %d %d\n",size,total_size,ret);
        }
        
        segmentsize=total_size/nr_segmente;
        snprintf(size, MAXBUF, "segment %d%s",segmentsize, delimitator);
        stream_write(sockfd[sockfdIndex], (void *)size,  MAXBUF);
        //sfarsit confirmare fisierul
       
        //scriu in buffer codul(get) si numele fisierului pe care il doresc
        snprintf(buf, MAXBUF, "get %d%s", offset, delimitator);
        
        //scriu in socket comanda
        //printf("Se trimite la server comanda %s",buf);
        stream_write(sockfd[sockfdIndex], (void *)buf, MAXBUF);
        
        //astept raspunsul serverului
        //ret = readline(sockfd[sockfdIndex], buf, MAXBUF);
        //printf("%i,%s",ret,buf);
        // if(ret != EX3_SUCCESS){
        //     printf("Client: Eroare raspuns. \n");
        //     exit(1);
        // }
        //preluarea fisierului
            
            //citirea fisierului
            toread+=segmentsize;
            //printf("Am ajuns si aici\n");
            while((nread = stream_read(sockfd[sockfdIndex], (void *)FILE_BUF+readoffset , toread)) > 0){
                readoffset+=nread;
                toread-=nread;
                //printf("BUF=%d\n",nread);
                
            }
           // printf("offset  = %i %i\n",readoffset,toread);
            offset+=segmentsize;
           if(nread < 0){
               printf("Client: Eroare la citire.\n");
               exit(1);
           }
        //sfarsitul preluarii
        
        sockfdIndex++;
        argvServerIndex++;
    }
    //scriem tot fisierul la final
    fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 00644); 
    
    if(fd == -1){
                printf("Client: Eroare la deschiderea fisierului\n");
                exit(1);
    }
    
    if(write(fd, (void *)FILE_BUF, total_size) == -1){
        printf("Client: Eroare la copierea in fisier.\n");
        exit(1);
    }
    close(fd);
    return 0;
    
}
