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
#include <linux/kernel.h>

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
    char tempStr[MSGLEN] = {0, };
    char portNum[MSGLEN] = "temp";
    char hostNum[MSGLEN] = {0, };
    char command[MSGLEN] = {0, };
    char* TOKEN = NULL;

    threadArgs args[USABLE_THREADS];   //arguments for thread
    pthread_t threads[USABLE_THREADS]; //threads


    int dataPort;         //데이터 전송용 포트는 명령어 포트에서 1씩 증가한 포트값을 사용
    int threadIdx = 0;          //스레드의 인덱스(스레드가 공유함)
    int threadIdxFIX;           //작업을 수행하다 인덱스가 바뀌면 안되니 고정

    //
    int getSpeed = DEFAULT_SPEED;
    int putSpeed = DEFAULT_SPEED;

    // if (argc != 3) {
    //     (void) fprintf(stderr,"usage: %s service|port host\n",argv[0]);
    //     exit(1);
    // }
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    for (;;)
    {
        printf("\n\n<usage 1> connect [IP/HOST] [PORTNUM]\n");
        printf("<usage 2> [STUDENTID]\n");
        printf("<usage 3> quit\n\n\n");
        

        fprintf(stderr, "> ");
        fflush(stderr);
        fgets(command, MSGLEN, stdin);
        command[strlen(command) - 1] = '\0';    //deleting \n

        if ( strncmp(command, "quit", 4) == 0 )
            exit(0);
        else if ( strncmp(command, "connect", 7) == 0)
        {
            strtok(command, " ");
            TOKEN = strtok(NULL, " ");
            strcpy(hostNum, TOKEN);

            TOKEN = strtok(NULL, " ");
            strcpy(portNum, TOKEN);

            if ((hostp = gethostbyname(hostNum)) == 0) 
            {
            fprintf(stderr,"%s: unknown host\n",hostNum);
            exit(1);
            }

            //printf("<<<<%s>>>>\n", inet_ntoa(*(struct in_addr*)hostp->h_addr_list[0]));

            /*while (!isdigit(portNum[0]))
            {
                fprintf(stderr, "put in port number : ");
                scanf("%s", portNum);
            }*/
            static struct servent s;
            servp = &s;
            s.s_port = htons((u_short)atoi(portNum));
            break;
        }

        else if(strncmp(command, "20133265", 8) == 0)
        {
            printf("\n\n================<20133265 CHA DONG MIN>===================\n");
            printf("* Implemented pthread in SendData, ReceiveData\n");
            printf("* Implemented struct tranSpeed[] \n");
            printf("* Little bit of interface command processing\n");
            printf("=========================================================\n\n");
        }

        printf("\n\n[WORNG COMMAND]");


    }



    // if (isdigit(argv[1][0])) {
    //     static struct servent s;
    //     servp = &s;
    //     s.s_port = htons((u_short)atoi(argv[1]));
    // } else if ((servp = getservbyname(argv[1], "tcp")) == 0) {
    //     fprintf(stderr,"%s: unknown service\n",atoi(argv[1]));
    //     exit(1);
    // }
    // if ((hostp = gethostbyname(argv[2])) == 0) {
    //     fprintf(stderr,"%s: unknown host\n",argv[2]);
    //     exit(1);
    // }
    memset((void *) &server, 0, sizeof server);
    server.sin_family = AF_INET;
    memcpy((void *) &server.sin_addr, hostp->h_addr, hostp->h_length);
    server.sin_port = servp->s_port;
    if (connect(sock, (struct sockaddr *)&server, sizeof server) < 0) {
        (void) close(sock);
        perror("connect");
        exit(1);
    }

    printf("[connected to server.. press ENTER key]\n");

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
            //fflush(stderr);
            printf("\n\n<usage 1> put [FILENAME]\n");
            printf("<usage 2> get [FILENAME]\n");
            printf("<usage 3> close \n\n");
            printf("> ");

            /* data from keyboard */
            if (!fgets(buf, sizeof buf, stdin)) {
                if (ferror(stdin)) {
                    perror("stdin");
                    exit(1);
                }
                exit(0);
            }

            if ( strncmp(buf, "quit", 4) == 0)
                exit(0);

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
                        strcpy(args[threadIdxFIX].serverIP, hostNum);

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
                        strcpy(args[threadIdxFIX].serverIP, hostNum);

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
    struct hostent *hostp;

    int listen_sock, // 소켓번호
    nbyte, nbuf;
    char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread;

    int finished = 0;       //finished = 1 means, file receving loop is to be finished

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
     hostp = gethostbyname(serverIP);

    memset((void *) &servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    memcpy((void *) &servaddr.sin_addr, hostp->h_addr, hostp->h_length);
    servaddr.sin_port = htons(portNum);

     // 에코 서버의 소켓주소 구조체 작성
     //servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = inet_addr(serverIP);
     //servaddr.sin_port = htons(portNum);
 
    sleep(1);

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


        while(!finished)
        {

            //만일, 파일이 모두 전송되어서 마지막 메시지가 온 것이라면 무조건 종료.
            if(total >= filesize) 
            {
                    printf("Sucessfully transferred. press ENTER key\n");
                    fclose(fp);      //stream 닫기
                      total = filesize;  //다 받은 것이나 마찬가지.
                     finished = 1;       //이제 모든 루프를 끝낸다
                     break;          //while문 빠져나가기

            }

            sread = recv( listen_sock, buf, BLOCK, 0 );

            total += sread;
            buf[sread] = 0;
            fwrite( buf, 1, sread, fp);
            bzero( buf, sizeof(buf) );

            
        }


        close(listen_sock);


    (*threadIdx) = (*threadIdx) - 1;

    free(buf);

    return 0;
}
 



//파일을 보내는 함수 (아직 미구현 상태)

 void* SendData(void *p)
 {
    struct sockaddr_in servaddr;
    struct hostent *hostp;

    int listen_sock, // 소켓번호
    nbyte, nbuf;
    char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread;

    time_t lastTime;        
    time_t currentTime;     //1초마다 nKB 위해 사용할 시간 변수


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

    sleep(1);

    if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
     perror("socket fail");
     exit(0);
    }
 
     // 에코 서버의 소켓주소 구조체 작성
     hostp = gethostbyname(serverIP);

    memset((void *) &servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    memcpy((void *) &servaddr.sin_addr, hostp->h_addr, hostp->h_length);
    servaddr.sin_port = htons(portNum);
 
    sleep(1);


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

    write( listen_sock, &filesize, sizeof(filesize));
 
    time(&lastTime);

    while(!feof(fp))
    {
        if((time(&currentTime) - lastTime) < 1)
            continue;

        else {

            sread = fread( buf, 1, BLOCK, fp );

            if(sread <= 0)
            {
                break;
            }

            total += sread;
            buf[sread] = 0;
            write( listen_sock, buf, sread);

            time(&lastTime);

        }
    }
 
    //if receiver got "endoffile", it will escape receiving loop
    //strcpy(buf, "endoffile");
    //write(listen_sock, buf, strlen("endoffile"));

    printf("Sucessfully transferred. press ENTER key\n");

    fclose(fp);
    close(listen_sock);

    (*threadIdx) = (*threadIdx) - 1;

    free(buf);

    return 0;

}
