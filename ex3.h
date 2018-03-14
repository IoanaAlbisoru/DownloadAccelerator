#ifndef EX3_H_
#define EX3_H_

#define EX3_SUCCESS 0
#define EX3_GOAHEAD 1
#define EX3_BYE 2
#define EX3_READERR 3
#define EX3_LONGLINE 4
#define EX3_FILECREA 5
#define EX3_FILEWRERR 6
#define EX3_EARLYEOF 7
#define EX3_FNAMNOTSET 8
#define EX3_INVCMD 9

int readline(int connfd, char*buf,int maxlen);

#endif