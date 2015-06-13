#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>

//
#define DEFAULT_SPEED 20
#define USABLE_THREADS 500
#define MSGLEN 128
#define FILE_BUFFER_SIZE (1024*1000)

//스레드를 실행할 때, 매개변수로 넣을 구조체.
typedef struct threadArgs
{
    char fname[MSGLEN];
    int speed;
    int portNum;
    char serverIP[MSGLEN];
    int* threadIdx;
} threadArgs;

void* ReceiveData(void* p);
void* SendData(void* p);

int main(int argc, char *argv[])
{
    struct hostent *hostp;
    struct servent *servp;
    struct sockaddr_in server;
    int sock;
    static struct timeval timeout = { 5, 0 }; /* five seconds */
    fd_set rmask, xmask, mask;
    char buf[BUFSIZ];
    char fname[MSGLEN] = {0, };
    int nfound, bytesread;
    char tempStr[64] = {0, };

    threadArgs args[USABLE_THREADS];   //arguments for thread
    pthread_t threads[USABLE_THREADS]; //threads


    int dataPort;         //데이터 전송용 포트는 명령어 포트에서 1씩 증가한 포트값을 사용
    int threadIdx = 0;          //스레드의 인덱스(스레드가 공유함)
    int threadIdxFIX;           //작업을 수행하다 인덱스가 바뀌면 안되니 고정

    //
    int getSpeed = DEFAULT_SPEED;
    int putSpeed = DEFAULT_SPEED;

    if (argc != 3) {
        (void) fprintf(stderr,"usage: %s service|port host\n",argv[0]);
        exit(1);
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    if (isdigit(argv[1][0])) {
        static struct servent s;
        servp = &s;
        s.s_port = htons((u_short)atoi(argv[1]));
    } else if ((servp = getservbyname(argv[1], "tcp")) == 0) {
        fprintf(stderr,"%s: unknown service\n",atoi(argv[1]));
        exit(1);
    }
    if ((hostp = gethostbyname(argv[2])) == 0) {
        fprintf(stderr,"%s: unknown host\n",argv[2]);
        exit(1);
    }
    memset((void *) &server, 0, sizeof server);
    server.sin_family = AF_INET;
    memcpy((void *) &server.sin_addr, hostp->h_addr, hostp->h_length);
    server.sin_port = servp->s_port;
    if (connect(sock, (struct sockaddr *)&server, sizeof server) < 0) {
        (void) close(sock);
        perror("connect");
        exit(1);
    }
    FD_ZERO(&mask);
    FD_SET(sock, &mask);
    FD_SET(fileno(stdin), &mask);
    for (;;) {
        rmask = mask;
        nfound = select(FD_SETSIZE, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
        if (nfound < 0) {
            if (errno == EINTR) {
                printf("interrupted system call\n");
                continue;
            }
            /* something is very wrong! */
            perror("select");
            exit(1);
        }
        if (FD_ISSET(fileno(stdin), &rmask)) {
            /* data from keyboard */
            if (!fgets(buf, sizeof buf, stdin)) {
                if (ferror(stdin)) {
                    perror("stdin");
                    exit(1);
                }
                exit(0);
            }

            buf[(strlen(buf) - 1)] = '\0';

            if (write(sock, buf, strlen(buf)) < 0) {
                perror("write");
                exit(1);
            }

            if(strncmp(buf, "put", 3) == 0)
            {           
                        //fix thread
                        threadIdxFIX = threadIdx;

                        strcpy(fname, buf + 4); //파일명 추출
                        strcpy(args[threadIdxFIX].fname, fname);
                        args[threadIdxFIX].threadIdx = &threadIdx;

                        threadIdx++;

                        //now, receive new portnum for file transfer
                        bytesread = read(sock, buf, sizeof buf);
                        buf[bytesread] = '\0';


                        memcpy(&dataPort, (int*)buf, 4);
                        args[threadIdxFIX].portNum = dataPort;
                        args[threadIdxFIX].speed = putSpeed;
                        strcpy(args[threadIdxFIX].serverIP, argv[2]);

                        printf("new port for transfer : %d\n", dataPort);

                        //run new thread
                        pthread_create(&threads[threadIdxFIX], NULL, SendData, (void*)&args[threadIdxFIX]);
                        
            }

            else if(strncmp(buf, "get", 3) == 0)
            {
                       //fix thread
                        threadIdxFIX = threadIdx;

                        strcpy(fname, buf + 4); //파일명 추출
                        strcpy(args[threadIdxFIX].fname, fname);
                        args[threadIdxFIX].threadIdx = &threadIdx;

                        threadIdx++;

                        //now, receive new portnum for file transfer
                        bytesread = read(sock, buf, sizeof buf);
                        buf[bytesread] = '\0';

                        memcpy(&dataPort, (int*)buf, 4);
                        args[threadIdxFIX].portNum = dataPort;
                        args[threadIdxFIX].speed = getSpeed;
                        strcpy(args[threadIdxFIX].serverIP, argv[2]);

                        printf("new port for transfer : %d\n", dataPort);

                        //run new thread
                        pthread_create(&threads[threadIdxFIX], NULL, ReceiveData, (void*)&args[threadIdxFIX]);
                
            }

            //sendrate ***K (put)
            else if(strncmp(buf, "sendrate", 8) == 0)
            {
                strcpy(tempStr, buf + 9);  //extract number part
                tempStr[(strlen(tempStr) - 1)] = '\0';  //throw away 'K';

                putSpeed = atoi(tempStr);
            }

            //recvrate (get)
            else if(strncmp(buf, "recvrate", 8) == 0)
            {
                strcpy(tempStr, buf+ 9);
                tempStr[(strlen(tempStr) - 1)] = '\0';  //throw away 'K';

                getSpeed = atoi(tempStr);
            }

            //yout speeds
            else if(strncmp(buf, "ratecurr", 8) == 0)
            {
                printf("send : %d, recv : %d\n", putSpeed, getSpeed);
            }

        }
        if (FD_ISSET(sock,&rmask)) {
            /* data from network */
            bytesread = read(sock, buf, sizeof buf);
            buf[bytesread] = '\0';
            printf("%s: got %d bytes: %s\n", argv[0], bytesread, buf);
        }
    }
} /* main - client.c */




//파일을 받는 함수 (아직 미구현 상태)
void* ReceiveData(void* p)
{
    struct sockaddr_in servaddr;
    int listen_sock, // 소켓번호
    nbyte, nbuf;
    char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread;

    FILE* fp;

    char serverIP[MSGLEN] = {0, };

    //unpack all datas
    threadArgs* args = (threadArgs*)p;
    int* threadIdx = NULL;
    int portNum = args->portNum;
    int BLOCK = args->speed; 

    strcpy(filename, args->fname); //filename
    strcpy(serverIP, args->serverIP);
    threadIdx = args->threadIdx;

    //nKB means 1024*n Bytes
    BLOCK = 1024 * BLOCK;



    if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
     perror("socket fail");
     exit(0);
     }
 
     // 에코 서버의 소켓주소 구조체 작성
     bzero((char *)&servaddr, sizeof(servaddr));
     servaddr.sin_family = AF_INET;
     servaddr.sin_addr.s_addr = inet_addr(serverIP);
     servaddr.sin_port = htons(portNum);
 
    // 연결요청
     if(connect(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
     perror("connect fail");
     exit(0);
    }

        

         if((fp = fopen(filename, "wb")) == 0)
    {
        printf("write file descriptor : FAILED\n");
        exit(0);
    }

    //getting filesize
    recv( listen_sock, buf, 4, 0);
    memcpy(&filesize, (int*)buf,4);

    printf("filename : %s, filesize : %d B\n", filename, filesize);



        printf("processing : ");
        while( total != filesize )
        {
            sread = recv( listen_sock, buf, BLOCK, 0 );
//            printf( "file is receiving now.. " );
            total += sread;
            buf[sread] = 0;
            fwrite( buf, 1, sread, fp);
            bzero( buf, sizeof(buf) );
            printf( "processing : %4.2f%% ", total*100 / (float)filesize );

            
        }

        fclose(fp);
        close(listen_sock);


    (*threadIdx) = (*threadIdx) - 1;

    free(buf);

    return 0;
}
 



//파일을 보내는 함수 (아직 미구현 상태)

 void* SendData(void *p)
 {
struct sockaddr_in servaddr;
    int listen_sock, // 소켓번호
    nbyte, nbuf;
    char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread;


    FILE* fp;

    char serverIP[MSGLEN] = {0, };

    //unpack all datas
    threadArgs* args = (threadArgs*)p;
    int* threadIdx = NULL;
    int portNum = args->portNum;
    int BLOCK = args->speed; 

    strcpy(filename, args->fname); //filename
    strcpy(serverIP, args->serverIP);
    threadIdx = args->threadIdx;

    //nKB means 1024*n Bytes
    BLOCK = 1024 * BLOCK;

printf("newport : %d, filename : %s, speed : %d, IP : %s\n", portNum, filename, BLOCK/1024, serverIP);

    if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
     perror("socket fail");
     exit(0);
     }
 
     // 에코 서버의 소켓주소 구조체 작성
     bzero((char *)&servaddr, sizeof(servaddr));
     servaddr.sin_family = AF_INET;
     servaddr.sin_addr.s_addr = inet_addr(serverIP);
     servaddr.sin_port = htons(portNum);
 
    // 연결요청
     if(connect(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
     perror("connect fail");
     exit(0);
    }



 if( (fp = fopen( filename, "rb" )) < 0 )
 {
 printf( "open failed" );
 exit(0);
 }
 
 fseek( fp, 0L, SEEK_END );
 filesize = ftell(fp);   //그 포인터 값이 바로 파일 크기이다.
 fseek(fp, 0L, SEEK_SET );

printf("filename : %s, filesize : %d B\n", filename, filesize);

 send( listen_sock, &filesize, sizeof(filesize), 0 );
 
 while( total != filesize )
 {
 sread = fread( buf, 1, BLOCK, fp );
 printf( "file is sending now.. " );
 total += sread;
 buf[sread] = 0;
 send( listen_sock, buf, sread, 0 );
 printf( "processing :%4.2f%% ", total*100 / (float)filesize );
 //        usleep(10000);
 }
 printf( "file translating is completed " );
 printf( "filesize : %d, sending : %d ", filesize, total );
 
 fclose(fp);
 close(listen_sock);

 (*threadIdx) = (*threadIdx) - 1;

free(buf);

 return 0;

}
