#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define SIZE    sizeof(struct sockaddr_in)

void closesock(int sig);
void respond(int n);

int sockfd_connect;
char* ROOT;
int main()
{
	int sockfd_listen, client_addr_size;
	char c[10000];
	struct sockaddr_in server = {AF_INET, htons(10000), htonl(INADDR_ANY)};
	struct sockaddr_in client;
	struct sigaction act;
	pid_t pid;

	act.sa_handler = closesock;
	sigfillset(&(act.sa_mask));
	sigaction(SIGPIPE, &act, NULL);
	
	ROOT = getenv("PWD");

	if((sockfd_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("fail to call socket()\n");
		exit(1);
	}

	printf("Server on - %s : %d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
	if(bind(sockfd_listen, (struct sockaddr *)&server, SIZE) == -1) {
		printf("fail to call bind()\n");
		exit(1);
	}
	
	if(listen(sockfd_listen, 5) == -1) {
		printf("fail to call listen()\n");
		exit(1);
	}
	
	while(1) {
		client_addr_size = sizeof(client);
		if((sockfd_connect = accept(sockfd_listen, (struct sockaddr*)&client, &client_addr_size)) == -1) {
			printf("fail to call accept()\n");
			continue;
		}
		else{
			pid = fork();
			switch(pid){
				case -1:
					puts("fork 실패 폭파합니다!");
					return -1;
					break;
				case 0:
					puts("연결되었습니다");
					puts(" ");
					respond(sockfd_connect);
					return 0;
					break;
				default:
					puts("자식을 생성합니다.");
					puts(" ");
			}
		}
	}
}

void closesock(int sig)
{
	close(sockfd_connect);
	printf("connection is lost\n");
	exit(0);
}

void respond(int n){
	char mesg[99999], *request[3], data_tosend[1024], path[99999];
	int rcvd, fd, bytes_read;
	memset((void*)mesg, (int)'\0', 99999);
	rcvd = recv(n, mesg, 99999, 0);

	if(rcvd < 0)
	{
		fprintf(stderr, "recv() error\n");
	}
	else if(rcvd == 0)
	{
		fprintf(stderr, "클라이언트와 연결이 끊겼습니다.\n");
	}
	else{
		printf("%s", mesg);
		puts(" ");
		puts(" ");
		request[0] = strtok(mesg, " \t\n");
		if(strncmp(request[0],"GET\0", 4) == 0)
		{
			request[1] = strtok(NULL, " \t");
			request[2] = strtok(NULL, " \t\n");
			if(strncmp(request[2], "HTTP/1.0", 8) != 0 && strncmp(request[2], "HTTP/1.1", 8) != 0)
			{
				write(n, "400 Bad Request\n", 25);
			}
			else
			{
				if(strncmp(request[1], "/\0", 2) == 0)
					request[1] = "/index.html";
				
				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], request[1]);
				printf("flie: %s\n", path);

				if((fd = open(path, O_RDONLY)) != -1)
				{
					send(n, "HTTP/1.1 200 OK\n\n", 17, 0);
					while((bytes_read = read(fd, data_tosend, 1024)) > 0)
						write(n, data_tosend, bytes_read);
				}
				else
				{
					write(n, "404 Not Found\n", 23);
				}
					
			}
		}
	}
	shutdown(n, SHUT_RDWR);
	close(n);
}
