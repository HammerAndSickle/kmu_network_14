#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <limits.h>

#define USABLE_THREADS 100
#define MSGLEN 128
#define CMD_BUFFER_SIZE 128
#define FILE_BUFFER_SIZE 2000

void buildSocket(int* socketDes, struct sockaddr_in* serverAddr, char* IPaddr, char* portNum);
void InitialConnect();
void SendCommand(char* IPaddr, char* portNum);
void ReceiveData();
void SendData();

void main()
{
    InitialConnect();
}

//처음 클라이언트 실행 시 커맨드 창. 서버 소켓과 연결하기 전.
//ex) connect 127.0.0.1 12345로 연결하거나, quit로 프로그램 종료 가능
void InitialConnect()
{
    char connectDATA[MSGLEN];			//서버의 아이피나 포트를 넣을 곳
    char* TOKEN = NULL;
    
    char IPString[MSGLEN];				//IP를 저장하는 문자열
    char PORTString[MSGLEN];			//포트를 저장하는 문자열
    
    while(1)
    {
        //여기서의 명령어는 connect, quit만이 허용된다.
        printf("> ");
        fgets(connectDATA, MSGLEN, stdin);
        
        if(strncmp(connectDATA, "connect", 7) == 0)
        {
            //입력한 문자열 중 토큰 ' '을 기준으로 나누어 IP와 포트 번호를 얻음.
            strtok(connectDATA, " ");
            TOKEN = strtok(NULL, " ");
            strcpy(IPString, TOKEN);
            
            TOKEN = strtok(NULL, " ");
            strcpy(PORTString, TOKEN);
            
            //연결이 끝났다면 이제 커맨드 모드로 접속
            SendCommand(IPString, PORTString);
        }
        
        
        //quit을 받으면 프로그램을 끝낸다.
        else if(strncmp(connectDATA, "quit", 4) == 0)
        {
            break;
        }
    }
}


//서버에게 명령어를 보내 처리받는 부분이다.
void SendCommand(char* IPaddr, char* portNum)
{
    int cmdSocketDes;					//통신에 사용하는 소켓
    struct sockaddr_in serverAddr;		//연결할 서버의 정보가 저장될 것이다.
    	int nfound;    
	int maxfd;	

    int nread;				//데이터의 크기
    char cmd_buffer[CMD_BUFFER_SIZE] = {0, };	//명령어 버퍼
    char fname[MSGLEN] = {0, };			//get이나 put으로 요구하는 파일 이름
    
	struct timeval timeout = {5, 0};
	fd_set rmask, xmask, mask;

    char* TOKEN = NULL;
    
    //토큰으로 분리해낸 아이피와 포트를 이용해 클라리언트 소켓을 만든다.
    buildSocket(&cmdSocketDes, &serverAddr, IPaddr, portNum);
    
    //위에서 만든 클라이언트의 소켓과, 위에서 얻은 서버의 정보를 이용해 서버에게 연결
    if(connect(cmdSocketDes, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1 )
    {
        printf("connect() error\n");
        perror("connect");
    }
    
	FD_ZERO(&mask);
	FD_SET(cmdSocketDes,&mask);
	FD_SET(fileno(stdin), &mask);
	maxfd = cmdSocketDes;

    while(1)
    {
		rmask = mask;
		nfound = select(FD_SETSIZE, &rmask, (fd_set*)0, (fd_set*)0, &timeout);

        printf("> ");
        
        //문자열을 입력받아 버퍼에 저장한다.
        if(!fgets(cmd_buffer, sizeof(cmd_buffer), stdin) < 0)
        {
            printf("ERROR : wrong input\n");
            exit(1);
        }
       

        //fgets는 개행 문자(\n)까지 얻어오므로 이를 제거한다.
        cmd_buffer[strlen(cmd_buffer) - 1] = '\0';
        
        //명령어는 get, put, close만을 허용한다.
        //그 외의 명령어는 아예 서버에게 보내지도 않을 것.
        
        //get [FILE]일 경우, 클라이언트는 서버에게서 파일을 받는다.
        if(strncmp(cmd_buffer, "get ", 4) == 0)
        {
            strtok(cmd_buffer, " ");
            TOKEN = strtok(NULL, " "); //파일명 추출
            strcpy(fname, TOKEN);
            
            //공백 문자는 sendto로 제대로 전달되지 않는다. 다른 문자로 잠깐 치환해서 보내도록 한다.
            cmd_buffer[3] = '0';
       
			if(FD_ISSET(fileno(stdin), &rmask))
			{
     
            //서버에게 명령어를 전달
            nread = write(cmdSocketDes, cmd_buffer, CMD_BUFFER_SIZE);
            		}
			if(FD_ISSET(cmdSocketDes, &rmask)	{

			nread = read(cmdSocketDes, cmd_buffer, sizeof(cmd_buffer));
			cmd_buffer[nread] = '\0';
		
		}

            printf("[%s]을 받아오는 함수.\n", fname);
            //------------receive 함수 스레드로 실행 구현---------
        }
        
        //put [FILE]일 경우, 클라이언트는 서버에게 파일을 보내준다.
        else if(strncmp(cmd_buffer, "put ", 4) == 0)
        {
            strtok(cmd_buffer, " ");
            TOKEN = strtok(NULL, " "); //파일명 추출
            strcpy(fname, TOKEN);
            
            //공백 문자는 sendto로 제대로 전달되지 않는다. 다른 문자로 잠깐 치환해서 보내도록 한다.
            cmd_buffer[3] = '0';
            
            //서버에게 명령어를 전달
            if(FD_ISSET(fileno(stdin), &rmask))
			{
     
            //서버에게 명령어를 전달
            nread = write(cmdSocketDes, cmd_buffer, CMD_BUFFER_SIZE);
            		}
			if(FD_ISSET(cmdSocketDes, &rmask)	{

			nread = read(cmdSocketDes, cmd_buffer, sizeof(cmd_buffer));
			cmd_buffer[nread] = '\0';
		
		}

		
		}

            printf("[%s]을 보내는 함수.\n", fname);
            //------------send 함수 스레드로 실행 구현---------
        }
        
        //close의 경우는, 소켓을 종료하고 다시 initialConnect  함수로 돌아갈 것.
        else if(strncmp(cmd_buffer, "close", 5) == 0)
        {
            printf("Disconnected.\n");
            close(cmdSocketDes);
            break;
        }
        
        //잘못된 커맨드
        else printf("WRONG COMMAND.\n\n");
    }
}

//클라이언트의 소켓을 만들고, 방금 연결할 때 사용한 서버의 정보를 구조체에 저장한다.
void buildSocket(int* socketDes, struct sockaddr_in* serverAddr, char* IPaddr, char* portNum)
{
    (*socketDes) = socket(PF_INET, SOCK_STREAM, 0);
    if( (*socketDes) == 0)
    {
        printf("socket() error\n");
        perror("socket");
    }
    
    memset(serverAddr, 0, sizeof((*serverAddr)));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = inet_addr(IPaddr);
    serverAddr->sin_port = htons(atoi(portNum));
    
}

/*
//파일을 보내는 함수 (아직 미구현 상태)
void SendData(char* IPaddr, char* portNum)
{
    struct sockaddr_in servaddr;
    int s, nbyte;
    char buf[MAXLINE+1];
    char filename[20];
    int filesize, fp, filenamesize ;
    int sread, total=0;
    
//    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
//    {
//        perror("socket fail");
//        exit(0);
//    }
//    
////    에코 서버의 소켓주소 구조체 작성
//    bzero((char *)&servaddr, sizeof(servaddr));
//    servaddr.sin_family = AF_INET;
//    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
//    servaddr.sin_port = htons(atoi(argv[2]));
    
    buildSocket(&s, servaddr, IPaddr, portNum);
    
//    연결요청
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
        usleep(10000);
    }
    printf( "file translating is completed " );
    printf( "filesize : %d, sending : %d ", filesize, total );
    
    close(fp);
    close(s);
    return 0;
}
 */

//파일을 받는 함수 (아직 미구현 상태)
/*void ReceiveData()
 {
 struct sockaddr_in servaddr, cliaddr;
 int listen_sock, accp_sock,  소켓번호
 addrlen = sizeof(cliaddr),  주소구조체 길이
 nbyte, nbuf;
 char buf[MSGLEN];
 char cli_ip[20];
 char filename[20];
 int filesize=0;
 int total=0, sread, fp;
 
 if(argc != 2) {
 printf("usage: %s port ", argv[0]);
 exit(0);
 }
  소켓 생성
 if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
 perror("socket fail");
 exit(0);
 }
  servaddr을 ''으로 초기화
 bzero((char *)&servaddr, sizeof(servaddr));
  servaddr 세팅
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 servaddr.sin_port = htons(atoi(argv[1]));
 
  bind() 호출
 if(bind(listen_sock, (struct sockaddr *)&servaddr,
 sizeof(servaddr)) < 0)
 {
 perror("bind fail");
 exit(0);
 }
  소켓을 수동 대기모드로 세팅
 listen(listen_sock, 5);
 
 while(1)
 {
 puts("서버가 연결요청을 기다림..");
  연결요청을 기다림
 accp_sock = accept(listen_sock,
 (struct sockaddr *)&cliaddr, &addrlen);
 
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
 recv( accp_sock, filename, sizeof(filename), 0 );
 printf( "%s ", filename );
 
  recv( accp_sock, &filesize, sizeof(filesize), 0 );
 read( accp_sock, &filesize, sizeof(filesize) );
 printf( "%d ", filesize );
 
 strcat( filename, "_backup" );
 fp = open( filename, O_WRONLY | O_CREAT | O_TRUNC);
 
 while( total != filesize )
 {
 sread = recv( accp_sock, buf, 100, 0 );
 printf( "file is receiving now.. " );
 total += sread;
 buf[sread] = 0;
 write( fp, buf, sread );
 bzero( buf, sizeof(buf) );
 printf( "processing : %4.2f%% ", total*100 / (float)filesize );
             usleep(1000);
 
 }
 printf( "file traslating is completed " );
 printf( "filesize : %d, received : %d ", filesize, total );
 
 close(fp);
 close(accp_sock);
 }
 
 close( listen_sock );
 return 0;
 }*/
