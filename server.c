#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define MAXB 500 //消息字符串最大长度
#define MAXC 3 //允许的最多客户端数
#define MAXU 10 //最大用户名长度
int check(int fd,char username[])
{
	FILE *fp;
	char ch;
	int i = 0;
	char uname[MAXU];
	if ((fp = fopen("user.txt", "r")) == NULL)
	{
		fp = fopen("user.txt", "w");
		fprintf(fp,"client%d %s\n",fd,username);
		fclose(fp);
		return 1;
	}
	else
	{
		while ((ch = fgetc(fp)) != EOF)
		{
			fseek(fp, -1, SEEK_CUR);
			while (fgetc(fp) != ' ');
			while ((ch = fgetc(fp)) != '\n')
				uname[i++] = ch;
			uname[i] = '\0';
			if (strcmp(username,uname) == 0)
			{
				fclose(fp);
				return 0;
			}
			else
			{
				i = 0;
				uname[0] = '\0';
			}
		}
		fclose(fp);
		fp = fopen("user.txt", "a");
		fprintf(fp,"client%d %s\n", fd,username);
		fclose(fp);
		return 1;
	}
}

int sendToClient(int fd,char* buf,int size)
{
	int i=0,j=0; //字符串数组下标
	FILE *fp;
	char ch;
	int dest,so;
	char dest_uname[MAXU],so_uname[MAXU];
	char target[MAXU],from[MAXU+10],info[MAXU+10+MAXB];
	while(buf[i++]!='"');
	while(buf[i++]!=' ');
	while(buf[i]!='\0')
		target[j++]=buf[i++];
	target[j]='\0';
	fp=fopen("user.txt","r");
	while(fgetc(fp)!=EOF){
		j=0;
		while(fgetc(fp)!=' ');
		while((ch=fgetc(fp))!='\n')
			dest_uname[j++]=ch;
		dest_uname[j]='\0';
		j=0;
		if(strcmp(dest_uname,target)==0)
		{
			printf("\t转发给用户:%s\n",target);
			fseek(fp,-(strlen(dest_uname)+3),SEEK_CUR);
			dest=fgetc(fp)-48;
			fp=fopen("user.txt","r");
			while((ch=fgetc(fp))!=EOF)
			{
				while(ch=fgetc(fp)!='t');
				so=fgetc(fp)-48;
				if(so==fd)
				{
					fseek(fp,1,SEEK_CUR);	
					while((ch=fgetc(fp))!='\n')
						so_uname[j++]=ch;
					so_uname[j]='\0';
					strcpy(from,"from ");
					strcat(from,so_uname);
					strcat(from,":");
					strcpy(info,from);
					strcat(info,buf);
					send(dest,info,strlen(info)-strlen(dest_uname)-1,0);
					break;
				}
				
			}
			fclose(fp);
			return 1;
		}
		else
			dest_uname[0]='\0';
	}
	return 0;
}
int main(int argc, char **argv)
{
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	struct sockaddr_in server_addr, client_addr;
	int result;	
	fd_set readfds,testfds;
	char buf[MAXB + 1];
	int i=0;
	socklen_t len;
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	//服务端固定在本机的9734端口
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	server_addr.sin_port = 9734;
	server_len=sizeof(server_addr);
	bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(server_sockfd, 5);
	FD_ZERO(&readfds);
	FD_SET(server_sockfd,&readfds);
	bzero(&server_addr, sizeof(server_addr));
	printf("server启动成功\n");
	remove("user.txt");
	while (1)
	{
		char ch;
		int fd;
		int nread;
		char uname[MAXU];
		int login;
		testfds = readfds;
		struct timeval timeout;
		printf("等待客户端连接\n");
		// 设置最大等待时间
	  	timeout.tv_sec = 1;  
		timeout.tv_usec = 0;
		result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
		//result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, &timeout);
		if (result==-1)
		{
			perror("server");
			exit(1);
		}
		for (fd = 0; fd < FD_SETSIZE; fd++)
		{
			if (FD_ISSET(fd, &testfds))
			{
				if (fd == server_sockfd)
				{				
					
					client_len = sizeof(client_addr);
					client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_addr, &client_len);						
					if(i<MAXC)
					{
						read(client_sockfd, uname, MAXU);
						if (check(client_sockfd,uname))
						{
							printf("client%d已连接\n",client_sockfd);
							FD_SET(client_sockfd, &readfds);
							read(client_sockfd,&login,1);
							login=1;
							write(client_sockfd,&login,1);
						}
						else
							close(client_sockfd);
					}
					else
					{
						read(client_sockfd,&login,1);
						login=2;
						close(client_sockfd);
					}
				
				}
				else
				{
					ioctl(fd, FIONREAD, &nread);
					if (nread == 0)
					{
						close(fd);
						FD_CLR(fd, &readfds);
						printf("client%d已断开连接\n", fd);						
					}
					else
					{
						sleep(1);
						// 开始处理每个新连接上的数据收发
						while (1)
						{
							if (FD_ISSET(fd, &readfds))
							{
								// 当前连接的socket上有消息到来则接收对方发过来的消息并显示
								bzero(buf, MAXB + 1);
								// 接收客户端的消息
								len = recv(fd, buf, MAXB, 0);
								if (len > 0)
									printf("client%d的消息:%s\n",fd,buf);
								else
								{
									if (len < 0)
										printf("接受client%d的消息失败\n\t错误信息：%d:%s\n",fd,errno,strerror(errno));
									else
										printf("client%d已断开连接\n",fd);
									break;
								}
								sendToClient(fd,buf,strlen(buf));
							}
							break;
						}
					}
				}
			}
		}
	}
	close(server_sockfd);
	remove("user.txt");
	return 0;
}
