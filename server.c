#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>

#define CLIENTS_NUM 5
#define USABLE_THREADS 500
#define MSGLEN 128
#define FILE_BUFFER_SIZE (1024*1000)

//
typedef struct tranSpeed
{
    int putSpeed;
    int getSpeed;
} tranSpeed;

//스레드를 실행할 때, 매개변수로 넣을 구조체.
typedef struct threadArgs
{
    char fname[MSGLEN];
    int speed;
    int portNum;
    int clientFD;
    int* threadIdx;
} threadArgs;

void* ReceiveData(void* p);
void* SendData(void* p);

int main(int argc, char *argv[])
{
    tranSpeed speeds[USABLE_THREADS];         //speeds for per client
    threadArgs args[USABLE_THREADS];   //arguments for thread
    pthread_t threads[USABLE_THREADS]; //threads

    int dataPort;         //데이터 전송용 포트는 명령어 포트에서 1씩 증가한 포트값을 사용
    int threadIdx = 0;          //스레드의 인덱스(스레드가 공유함)
    int threadIdxFIX;           //작업을 수행하다 인덱스가 바뀌면 안되니 고정


    struct servent *servp;
    struct sockaddr_in server, remote;
    int request_sock, new_sock;
    int nfound, fd, maxfd, bytesread, addrlen;
    fd_set rmask, mask;
    static struct timeval timeout = { 5, 0 }; /* 5 seconds */
	char fname[128] = {0, };
    int tempValue = 0;
    char tempStr[64] = {0, };
    
    int i;

    char buf[BUFSIZ];
    if (argc != 2) {
        (void) fprintf(stderr,"usage: %s service|port\n",argv[0]);
        exit(1);
    }

    //initializtionf of speeds with 20
    for(i = 0; i < USABLE_THREADS; i++)
        speeds[i].putSpeed = speeds[i].getSpeed = 20;


    if ((request_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    if (isdigit(argv[1][0])) {
        static struct servent s;
        servp = &s;
        s.s_port = htons((u_short)atoi(argv[1]));
    } else if ((servp = getservbyname(argv[1], "tcp")) == 0) {
        fprintf(stderr,"%s: unknown service\n", "tcp");
        exit(1);
    }

    dataPort = atoi(argv[1]);

    memset((void *) &server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = servp->s_port;
    if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(request_sock, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    }
    FD_ZERO(&mask);
    FD_SET(request_sock, &mask);
    maxfd = request_sock;
    for (;;) {
        rmask = mask;
        nfound = select(maxfd+1, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
        if (nfound < 0) {
            if (errno == EINTR) {
                printf("interrupted system call\n");
                continue;
            }
            /* something is very wrong! */
            perror("select");
            exit(1);
        }
        if (FD_ISSET(request_sock, &rmask)) {
            /* a new connection is available on the connetion socket */
            addrlen = sizeof(remote);
            new_sock = accept(request_sock,
                              (struct sockaddr *)&remote, (socklen_t*) &addrlen);
            if (new_sock < 0) {
                perror("accept");
                exit(1);
            }
            printf("connection from host %s, port %d, socket %d\n",
                   inet_ntoa(remote.sin_addr), ntohs(remote.sin_port),
                   new_sock);
            FD_SET(new_sock, &mask);
            if (new_sock > maxfd)
                maxfd = new_sock;
            FD_CLR(request_sock, &rmask);
        }
        for (fd=0; fd <= maxfd ; fd++) {
            /* look for other sockets that have data available */
            if (FD_ISSET(fd, &rmask)) {
                /* process the data */
                bytesread = read(fd, buf, sizeof buf - 1);
                buf[bytesread] = '\0';
			



                        //get [filename]이라면, filename 추출 후 파일을 클라이언트에게 보내준다.
       	                if(strncmp(buf, "get", 3) == 0)
		        {
                                strcpy(fname, buf + 4); //파일명 추출
                                //printf("[%s]을 보내주는 함수.\n", fname);
                                //------------send 함수 스레드로 실행 구현---------

                                threadIdxFIX = threadIdx;
                                strcpy(args[threadIdxFIX].fname, fname);
                                args[threadIdxFIX].threadIdx = &threadIdx;
                                args[threadIdxFIX].speed = speeds[fd].getSpeed;
                                args[threadIdxFIX].clientFD = fd;

                                threadIdx++;

                                //now, we have to send new port num
                                args[threadIdxFIX].portNum = ++dataPort;
                                memcpy((int*)buf, &dataPort, 4);

                                printf("new port for transfer : %d\n", args[threadIdxFIX].portNum);
                                write(fd, buf, 4);

                                pthread_create(&threads[threadIdxFIX], NULL, SendData, (void*)&args[threadIdxFIX]);
            
        		}
        
                        //put [filename]이라면, filename 추출 후 파일을 클라이언트에게서 받는다.
   	                else if(strncmp(buf, "put", 3) == 0)
        	       {
                                 strcpy(fname, buf + 4); //파일명 추출
                                 //printf("[%s]을 받는 함수.\n", fname);
                                //------------receive 함수 스레드로 실행 구현--------

                                
                                 threadIdxFIX = threadIdx;
                                strcpy(args[threadIdxFIX].fname, fname);
                                args[threadIdxFIX].threadIdx = &threadIdx;
                                args[threadIdxFIX].speed = speeds[fd].putSpeed;
                                args[threadIdxFIX].clientFD = fd;

                                threadIdx++;

                                //now, we have to send new port num
                                args[threadIdxFIX].portNum = ++dataPort;
                                memcpy((int*)buf, &dataPort, 4);

                                printf("new port for transfer : %d\n", args[threadIdxFIX].portNum);
                                write(fd, buf, 4);

                                pthread_create(&threads[threadIdxFIX], NULL, ReceiveData, (void*)&args[threadIdxFIX]);
                                
     		          }

                //sendrate (put)
               else if(strncmp(buf, "sendrate", 8) == 0)
               {
                    //bring integer from buffer. As for"sendrate 40", value 40 begins at buffer[9].
                    strcpy(tempStr, buf + 9);  //extract number part
                     tempStr[(strlen(tempStr) - 1)] = '\0';  //throw away 'K';

                    speeds[fd].putSpeed = atoi(tempStr);

                    printf("client %d's send : %d, recv : %d\n", fd, speeds[fd].putSpeed, speeds[fd].getSpeed);

               }

            //recvrate (get)
               else if(strncmp(buf, "recvrate", 8) == 0)
               {
                    //bring integer from buffer. As for"sendrate 40", value 40 begins at buffer[9].
                    strcpy(tempStr, buf + 9);  //extract number part
                    tempStr[(strlen(tempStr) - 1)] = '\0';  //throw away 'K';

                    speeds[fd].getSpeed = atoi(tempStr);

                    printf("client %d's send : %d, recv : %d\n", fd, speeds[fd].putSpeed, speeds[fd].getSpeed);

               }


            


                if (bytesread<0) {
                    perror("read");
                    /* fall through */
                }
                if (bytesread<=0) {
                    printf("server: end of file on %d\n",fd);
                    FD_CLR(fd, &mask);
                    if (close(fd)) perror("close");
                    continue;
                }
                buf[bytesread] = '\0';
                printf("%s: %d bytes from %d: %s\n",
                       argv[0], bytesread, fd, buf);
                
            }
        }
    }
} /* main - server.c */



//파일을 받는 함수 (아직 미구현 상태)
void* ReceiveData(void* p)
{
     
 struct sockaddr_in servaddr;
 struct sockaddr_in cliaddr;
int addrlen = sizeof(cliaddr);

 int listen_sock, accp_sock, nbyte;
char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
 char filename[20];
 int filesize, filenamesize ;

    time_t lastTime;        
    time_t currentTime;     //1초마다 상황 출력을 위해 사용할 시간 변수

int finished = 0;       //finished = 1 means, file receving loop is to be finished
int clientFD;

 FILE* fp;
 int sread, total=0;
 
    //unpack all datas
    threadArgs* args = (threadArgs*)p;
    int* threadIdx = NULL;
    int portNum = args->portNum;
    int BLOCK = args->speed; 

    strcpy(filename, args->fname); //filename
    threadIdx = args->threadIdx;
    clientFD = args->clientFD;

    //nKB means 1024*n Bytes
    BLOCK = 1024 * BLOCK;

 
     if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
    perror("socket fail");
    exit(0);
    }
 
    printf("newport : %d, filename : %s, speed : %d\n", portNum, filename, BLOCK/1024);

 // 에코 서버의 소켓주소 구조체 작성
 bzero((char *)&servaddr, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 //inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
 servaddr.sin_port = htons(portNum);
 
    //bind() 호출
    if(bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
   {
        perror("bind fail");
        exit(0);
    }

listen(listen_sock, 5);

    accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);

    if((fp = fopen(filename, "wb")) == 0)
    {
        printf("write file descriptor : FAILED\n");
        exit(0);
    }

    //getting filesize
    recv( accp_sock, buf, 4, 0);
    memcpy(&filesize, (int*)buf,4);

    printf("filename : %s, filesize : %d B\n", filename, filesize);

        //get current time
        time(&lastTime);

        printf("processing : ");
        while(!finished)
        {

            //만일, 파일이 모두 전송되어서 마지막 메시지가 온 것이라면 무조건 종료.
            if(total >= filesize) 
            {
                    printf("Sucessfully transferred.\n");
                    fclose(fp);      //stream 닫기
                      total = filesize;  //다 받은 것이나 마찬가지.
                     finished = 1;       //이제 모든 루프를 끝낸다
                     break;          //while문 빠져나가기

            }

            sread = recv( accp_sock, buf, BLOCK, 0 );
            total += sread;

               //현 시간 - 아까 기록한 시간의 차가 1 이상이면 1초가 흐른 것으로 간주.
             if((time(&currentTime) - lastTime) >= 1)
              {
                  printf("Transfer status : recv[%s][%d%c , %4.2f MB / %4.2f MB] from socket %d\n", filename, (int)((double)total* 100 /filesize),'%', (double)total/1000000, (double)filesize/1000000, clientFD);
                 time(&lastTime);
                }


            buf[sread] = 0;
            fwrite( buf, 1, sread, fp);
            bzero( buf, sizeof(buf) );
            //printf( "processing : %4.2f%% ", total*100 / (float)filesize );

            
        }


        close(accp_sock);
        close(listen_sock);

        (*threadIdx) = (*threadIdx) - 1;

        free(buf);

        return 0;


}
 



//파일을 보내는 함수 (아직 미구현 상태)

 void* SendData(void *p)
 {
 struct sockaddr_in servaddr;
 struct sockaddr_in cliaddr;
int addrlen = sizeof(cliaddr);

 int listen_sock, accp_sock, nbyte;
 char* buf = (char*)calloc(FILE_BUFFER_SIZE, sizeof(char));
 char filename[20];
 int filesize, filenamesize ;
int clientFD;

 FILE* fp;
 int sread, total=0;

    time_t lastTime;        
    time_t currentTime;     //1초마다 상황 출력을 위해 사용할 시간 변수
 
    //unpack all datas
    threadArgs* args = (threadArgs*)p;
    int* threadIdx = NULL;
    int portNum = args->portNum;
    int BLOCK = args->speed; 

    strcpy(filename, args->fname); //filename
    threadIdx = args->threadIdx;
    clientFD = args->clientFD;

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
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 //inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
 servaddr.sin_port = htons(portNum);
 
    //bind() 호출
    if(bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
   {
        perror("bind fail");
        exit(0);
    }

listen(listen_sock, 5);

    accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);

 
 
 if( (fp = fopen( filename, "rb" )) < 0 )
 {
 printf( "open failed" );
 exit(0);
 }
 
 fseek( fp, 0L, SEEK_END );
 filesize = ftell(fp);   //그 포인터 값이 바로 파일 크기이다.
 fseek(fp, 0L, SEEK_SET );

printf("filename : %s, filesize : %d B\n", filename, filesize);

 write( accp_sock, &filesize, sizeof(filesize));
 
 //get current time
time(&lastTime);

 while(!feof(fp))
 {

        //현 시간 - 아까 기록한 시간의 차가 1 이상이면 1초가 흐른 것으로 간주.
        if((time(&currentTime) - lastTime) < 1)
            continue;

        printf("Transfer status : recv[%s][%d%c , %4.2f MB / %4.2f MB] from socket %d\n", filename, (int)((double)total* 100 /filesize),'%', (double)total/1000000, (double)filesize/1000000, clientFD);

        sread = fread( buf, 1, BLOCK, fp );

        if(sread <= 0)
        {
             break;
        }
 
        total += sread;

//현 시간 - 아까 기록한 시간의 차가 1 이상이면 1초가 흐른 것으로 간주.
/*        if((time(&currentTime) - lastTime) >= 1)
        {
            printf("Transfer status : send[%s][%d%c , %4.2f MB / %4.2f MB]\n", filename, (int)((double)total* 100 /filesize),'%', (double)total/1000000, (double)filesize/1000000);
            time(&lastTime);
        }
*/

        buf[sread] = 0;
        write( accp_sock, buf, sread);

        time(&lastTime);

 }


 
 //if receiver got "endoffile", it will escape receiving loop
 //strcpy(buf, "endoffile");
//write(accp_sock, buf, strlen("endoffile"));



 fclose(fp);
 close(accp_sock);
 close(listen_sock);


printf( "file translating is completed == " );
 printf( "filesize : %d, sending : %d ", filesize, total );


 (*threadIdx) = (*threadIdx) - 1;

free(buf);

 return 0;


}

