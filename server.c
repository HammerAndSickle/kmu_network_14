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
#define USABLE_THREADS 100
#define MSGLEN 128
#define CMD_BUFFER_SIZE 128
#define FILE_BUFFER_SIZE 2000

//
struct tranSpeed
{
    int putSpeed;
    int getSpeed;
};

//스레드를 실행할 때, 매개변수로 넣을 구조체.
typedef struct threadArgs
{
    char fname[MSGLEN];
    int speed;
    int dataSocketDes;
    struct sockaddr_in dataAddr;
    socklen_t data_addr_size;
    int* threadIdx;
} threadArgs;

void ReceiveData();
void SendData();

int main(int argc, char *argv[])
{
    struct tranSpeed speeds[100];         //speeds for per client
    struct threadArgs args[100 * 10];   //arguments for thread

    int dataPort;         //데이터 전송용 포트는 명령어 포트에서 1씩 증가한 포트값을 사용
    int threadIdx = 0;          //5개 스레드의 인덱스(스레드가 공유함)
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
    for(i = 0; i < 100; i++)
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
            printf("[%s]을 보내주는 함수.\n", fname);
            //------------send 함수 스레드로 실행 구현---------
            
        		}
        
        //put [filename]이라면, filename 추출 후 파일을 클라이언트에게서 받는다.
   	     if(strncmp(buf, "put", 3) == 0)
      		  {
            strcpy(fname, buf + 4); //파일명 추출
            printf("[%s]을 받는 함수.\n", fname);
            //------------receive 함수 스레드로 실행 구현--------
     		   }

                //sendrate (put)
               if(strncmp(buf, "sendrate", 8) == 0)
               {
                    //bring integer from buffer. As for"sendrate 40", value 40 begins at buffer[9].
                    strcpy(tempStr, buf + 9);  //extract number part
                     tempStr[(strlen(tempStr) - 1)] = '\0';  //throw away 'K';

                    speeds[fd].putSpeed = atoi(tempStr);

                    printf("client %d's send : %d, recv : %d\n", fd, speeds[fd].putSpeed, speeds[fd].getSpeed);

               }

            //recvrate (get)
                if(strncmp(buf, "recvrate", 8) == 0)
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
                /* echo it back */
                if (write(fd, buf, bytesread)!=bytesread)
                    perror("echo");
            }
        }
    }
} /* main - server.c */


/*
//파일을 받는 함수 (아직 미구현 상태)
void ReceiveData(char* portNum)
{
    struct sockaddr_in servaddr, cliaddr;
    int listen_sock, accp_sock, // 소켓번호
    addrlen = sizeof(cliaddr), // 주소구조체 길이
    nbyte, nbuf;
    char buf[MSGLEN];
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread, fp;
    
//    // 소켓 생성
//    if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
//        perror("socket fail");
//        exit(0);
//    }
//    // servaddr을 ''으로 초기화
//    bzero((char *)&servaddr, sizeof(servaddr));
//    // servaddr 세팅
//    servaddr.sin_family = AF_INET;
//    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
//    servaddr.sin_port = htons(atoi(argv[1]));
//    
//    // bind() 호출
//    if(bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
//    {
//        perror("bind fail");
//        exit(0);
//    }
    
    socketAndBind(&listen_sock, servaddr, portNum);
    
    // 소켓을 수동 대기모드로 세팅
    listen(listen_sock, 5);
    
    while(1)
    {
        puts("서버가 연결요청을 기다림..");
        // 연결요청을 기다림
        accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);
        
        if(accp_sock < 0)
        {
            perror("accept fail");
            exit(0);
        }
        
        puts("클라이언트가 연결됨..");
        
        inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cli_ip, sizeof(cli_ip));
        printf( "IP : %s ", cli_ip );
        printf( "Port : %x ", ntohs( cliaddr.sin_port) );
        
        bzero( filename, 20 );
//        recv( accp_sock, filename, sizeof(filename), 0 );
        read( accp_sock, filename, sizeof(filename) );
        printf( "%s ", filename );
        
        // recv( accp_sock, &filesize, sizeof(filesize), 0 );
        read( accp_sock, &filesize, sizeof(filesize) );
        printf( "%d ", filesize );
        
        strcat( filename, "_backup" );
        fp = open( filename, O_WRONLY | O_CREAT | O_TRUNC);
        
        printf("processing : ");
        while( total != filesize )
        {
            sread = recv( accp_sock, buf, 100, 0 );
//            printf( "file is receiving now.. " );
            total += sread;
            buf[sread] = 0;
            write( fp, buf, sread );
            bzero( buf, sizeof(buf) );
            printf( "processing : %4.2f%% ", total*100 / (float)filesize );

            
        }
        printf( "file traslating is completed " );
        printf( "filesize : %d, received : %d ", filesize, total );
        
        close(fp);
        close(accp_sock);
    }
    
    close( listen_sock );
    return 0;
}
 */



//파일을 보내는 함수 (아직 미구현 상태)
/*
 void SendData()
 {
 struct sockaddr_in servaddr;
 int s, nbyte;
 char buf[MAXLINE+1];
 char filename[20];
 int filesize, fp, filenamesize ;
 int sread, total=0;
 
 if(argc != 3)
 {
 printf("usage: %s ip_address port ", argv[0]);
 exit(0);
 }
 
 if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
 {
 perror("socket fail");
 exit(0);
 }
 
 // 에코 서버의 소켓주소 구조체 작성
 bzero((char *)&servaddr, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
 servaddr.sin_port = htons(atoi(argv[2]));
 
 // 연결요청
 if(connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
 {
 perror("connect fail");
 exit(0);
 }
 
 printf("select file to send : ");
 if ( fgets(filename, sizeof(filename), stdin) == NULL )
 exit(0);
 
 filenamesize = strlen(filename);
 filename[filenamesize-1] = 0;
 
 if( (fp = open( filename, O_RDONLY )) < 0 )
 {
 printf( "open failed" );
 exit(0);
 }
 
 send( s, filename, sizeof(filename), 0 );
 
 filesize = lseek( fp, 0, SEEK_END );
 send( s, &filesize, sizeof(filesize), 0 );
 lseek(fp, 0, SEEK_SET );
 
 while( total != filesize )
 {
 sread = read( fp, buf, 100 );
 printf( "file is sending now.. " );
 total += sread;
 buf[sread] = 0;
 send( s, buf, sread, 0 );
 printf( "processing :%4.2f%% ", total*100 / (float)filesize );
 //        usleep(10000);
 }
 printf( "file translating is completed " );
 printf( "filesize : %d, sending : %d ", filesize, total );
 
 close(fp);
 close(s);
 return 0;
 }*/
