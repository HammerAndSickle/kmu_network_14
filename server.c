#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE 127

int main(int argc, char *argv[])
{
    struct sockaddr_in servaddr, cliaddr;
    int listen_sock, accp_sock, // 소켓번호
    addrlen = sizeof(cliaddr), // 주소구조체 길이
    nbyte, nbuf;
    char buf[MAXLINE+1];
    char cli_ip[20];
    char filename[20];
    int filesize=0;
    int total=0, sread, fp;
    
    if(argc != 2) {
        printf("usage: %s port ", argv[0]);
        exit(0);
    }
    // 소켓 생성
    if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket fail");
        exit(0);
    }
    // servaddr을 ''으로 초기화
    bzero((char *)&servaddr, sizeof(servaddr));
    // servaddr 세팅
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
    // bind() 호출
    if(bind(listen_sock, (struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0)
    {
        perror("bind fail");
        exit(0);
    }
    // 소켓을 수동 대기모드로 세팅
    listen(listen_sock, 5);
    
    while(1)
    {
        puts("서버가 연결요청을 기다림..");
        // 연결요청을 기다림
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
        
        // recv( accp_sock, &filesize, sizeof(filesize), 0 );
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
//            usleep(1000);
            
        } 
        printf( "file traslating is completed " ); 
        printf( "filesize : %d, received : %d ", filesize, total );
        
        close(fp); 
        close(accp_sock); 
    }
    
    close( listen_sock ); 
    return 0; 
}