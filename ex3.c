#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

#include "ex3.h"

#define DELIM1 '\r'
#define DELIM2 '\n'

int readline(int connfd, char *buf, int maxlen){
    char *pos, *last;
#ifdef DELIM2
    int flag = 0;
#endif
    
    for(pos = buf, last = buf + maxlen; pos < last; pos++)
        switch(read(connfd, (void *)pos, 1)){
            case -1: return EX3_READERR;
            case 0: return EX3_EARLYEOF;
            default: if(*pos == DELIM1){
#ifdef DELIM2
                flag = 1;
                break;
            }
            if(*pos == DELIM2){
                if(!flag)
                    break;
                *(pos - 1) = '\0';
#else
*pos = '\0';
#endif
return EX3_SUCCESS;
            }
#ifdef DELIM2
if(flag)
    flag = 0;
#endif
break;
            }
        return EX3_LONGLINE;
}

