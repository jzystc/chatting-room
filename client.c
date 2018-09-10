#include <stdio.h>  
#include <string.h>  
#include <errno.h>  
#include <sys/socket.h>  
#include <resolv.h>  
#include <stdlib.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#define MAXB 500  
#define MAXU 10
int main(int argc, char **argv)  
{  
    int server_sockfd, len;  
    int login=0; //登录是否成功的标志
    struct sockaddr_in server_addr;  
    char buffer[MAXB + 1];  
    fd_set readfds;  
    struct timeval timeout;  
    int result;  
    if (argc != 2)   
    {  
        printf("参数格式错误！格式如下：%s 用户名\n",argv[0]);  
        exit(0);  
    }  
    // 创建一个 socket 用于 tcp 通信   
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)   
    {  
        perror("Socket");  
        exit(errno);  
    }  
      
    // 初始化服务器端的地址和端口
    bzero(&server_addr, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
   	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
   	server_addr.sin_port = 9734;
       
    if (connect(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1)   
    {  
        perror("connect ");  
        exit(errno);  
    }  
    char uname[MAXU];
    strcpy(uname,argv[1]);
    write(server_sockfd,uname,MAXU);  
    write(server_sockfd,&login,1);
    read(server_sockfd, &login,1);
    switch(login)
    {    
		case 1:
            printf("登录成功，可以开始聊天了，输入quit退出\n");  
            printf("按照格式输入消息可以将消息转发给指定用户，消息格式：\"消息内容\" 用户名\n");
            break;
        break;
        case 0:
            printf("登录失败，存在重名用户\n");  
            break;
        case 2:
            printf("登录失败，超过最大连接数\n");
            break;
	}
    while (1)   
    {    
        FD_ZERO(&readfds);  
        // 把标准输入句柄0加入到集合中   
        FD_SET(0, &readfds);  
        // 把server_sockfd加入到集合中   
        FD_SET(server_sockfd, &readfds);  
        // 设置等待时间   
        timeout.tv_sec = 1;  
        timeout.tv_usec = 0;     
        // 开始等待   
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);  
        if (result == -1)   
            break;  
        else if (result == 0)   
            //没有任何消息到来，用户也没有按键，继续等待 
            continue;  
        else   
        {  
            if (FD_ISSET(server_sockfd, &readfds))   
            {  
                // 接收消息   
                bzero(buffer, MAXB + 1);  
                len = recv(server_sockfd, buffer, MAXB, 0);  
                if (len > 0)  
                    printf("%s\n",buffer);  
                else   
                {  
                    if (len < 0)  
                        printf("接收消息失败！错误代码是%d，错误信息是'%s'\n",errno, strerror(errno));  
                    else  
                        printf("无法连接server\n");  
                    break;  
                }  
            }  
            if (FD_ISSET(0, &readfds))   
            {  
                // 读取用户输入的消息   
                bzero(buffer, MAXB + 1);  
                fgets(buffer, MAXB, stdin);  
                if (!strncasecmp(buffer, "quit", 4))   
                {  
                    printf("断开连接成功\n");  
                    break;  
                }          
                // 发送消息   
                len = send(server_sockfd, buffer, strlen(buffer) - 1, 0);  
                if (len < 0)   
                {  
                    printf("发送状态：失败\n");  
                    break;  
                }   
                else  
                    printf("发送状态：成功\n");  
            }  
        }  
    }  
    close(server_sockfd);  
    return 0;  
}  
