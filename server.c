#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define CLIENTS_NUM 5
#define USABLE_THREADS 100
#define MSGLEN 128
#define CMD_BUFFER_SIZE 128
#define FILE_BUFFER_SIZE 2000

void socketAndBind(int* socketDes, struct sockaddr_in* serverAddr, char* portNum);
void ReceiveCommand(char* initialPort);
void ReceiveData();
void SendData();


void main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("%s port\n", argv[0]);
        return;
    }
    
    ReceiveCommand(argv[1]);
    
}

//명령어를 입력받아 처리해 줄 함수이다. InitialPorst는 명령어 처리 소켓의 포트 번호이다
void ReceiveCommand(char* initialPort)
{
    int SocketDes;	//서버를 나타내는 소켓
    int cmdSocketDes;		//클라이언트와의 명령어 통신을 위한 새 소켓
    
    int nread;				//데이터의 크기
    char cmd_buffer[CMD_BUFFER_SIZE] = {0, };	//명령어 버퍼
    char fname[MSGLEN] = {0, };			//get이나 put으로 요구하는 파일 이름
    
    struct sockaddr_in serverAddr;		//새로운 데이터 소켓을 위한 서버 자신의 주소
    struct sockaddr_in clientAddr;		//명령어 연결을 위한 클라이언트  소켓
    socklen_t client_addr_size;			//상대방 주소
    
    client_addr_size = sizeof(clientAddr);
    
    //주어진 포트 번호로 명령어 전달용 소켓을 만든다. 클라이언트들은 일단 여기에 접속할 것.
    socketAndBind(&serverSocketDes, &serverAddr, initialPort);
    
    //이제 클라이언트들의 명령을 받아 처리한다.
    printf("* 서버가 연결요청을 기다림..\n");
    
    //클라이언트들의 접속을 기다린다.
    if(listen(serverSocketDes,  CLIENTS_NUM) == -1)
    {
        printf("listen() error\n");
        perror("listen");
    }
    
    //클라이언트의 연결을 받아들인다.
    cmdSocketDes = accept(serverSocketDes, (struct sockaddr *)&clientAddr, &client_addr_size);
    
    if(cmdSocketDes < 0)
    {
        perror("accept fail");
        exit(0);
    }
    
    
    while(1)
    {
        
        
        //명령어를 받아온다. 이를 해석한다.
        nread = read(cmdSocketDes, cmd_buffer, CMD_BUFFER_SIZE);
        cmd_buffer[nread] = '\0';
        
        
        //get [filename]이라면, filename 추출 후 파일을 클라이언트에게 보내준다.
        if(strncmp(cmd_buffer, "get", 3) == 0)
        {
            strcpy(fname, cmd_buffer + 4); //파일명 추출
            printf("[%s]을 보내주는 함수.\n", fname);
            //------------send 함수 스레드로 실행 구현---------
            
        }
        
        //put [filename]이라면, filename 추출 후 파일을 클라이언트에게서 받는다.
        if(strncmp(cmd_buffer, "put", 3) == 0)
        {
            strcpy(fname, cmd_buffer + 4); //파일명 추출
            printf("[%s]을 받는 함수.\n", fname);
            //------------receive 함수 스레드로 실행 구현--------
        }
        //이제 방금 연결 소켓은 닫아버린다.
        //close(cmdSocketDes);
    }
}

//소켓구조체와 소켓 디스크립터에 주어진 포트 번호를 할당해 소켓을 만들 바인딩시킨다.
void socketAndBind(int* socketDes, struct sockaddr_in* serverAddr, char* portNum)
{
    (*socketDes) = socket(PF_INET, SOCK_STREAM, 0);
    if( (*socketDes) == 0)
    {
        printf("socket() error\n");
        perror("socket");
    }
    
    memset(serverAddr, 0, sizeof((*serverAddr)));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr->sin_port = htons(atoi(portNum));
    
    if(bind((*socketDes), (struct sockaddr*)serverAddr, sizeof((*serverAddr))) == -1)
    {
        printf("bind() error\n");
        perror("bind");
    }
}
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
