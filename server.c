#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "define.h"

#define MAX_BUF 256
#define WIDTH = 1024
#define HEIGHT = 600

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int len;
	int sfd;
	struct sockaddr_in addr_server;
	struct sockaddr_in addr_client;
	socklen_t addr_client_len;
	char buf[MAX_BUF];

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_server.sin_port = htons(SERVER_PORT);
	ret = bind(sfd, (struct sockaddr *)&addr_server, sizeof(addr_server));
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
    
    printf("[%d] SERVER_PORT: %d\n", pid, SERVER_PORT);
	
	
	char player1_Msg[MAX_BUF];
	char player2_Msg[MAX_BUF];
	
	for(;;) {
		addr_client_len = sizeof(addr_client);
		len = recvfrom(sfd, buf, MAX_BUF-1, 0, (struct sockaddr *)&addr_client, &addr_client_len);
		
		if(len > 0) {
			buf[len] = 0;
			char client_ip[30];		
			inet_ntop(AF_INET, &addr_client.sin_addr, client_ip, INET_ADDRSTRLEN);
			int code = client_ip[11] - '0';
			
			/* Split 2 Clients IP */
			if(code == 3){
				strcpy(player1_Msg, buf);
				strcat(player1_Msg, " 3"); //Send Current Client's Code;			
				//sprintf(player1_Msg, "%s%s%d", player1_Msg, " ", 3);
				printf("[%d] send: %s from: %s\n", pid, player2_Msg, client_ip);  
				/* echo back */
				sendto(sfd, player2_Msg, len + 2, 0,  (struct sockaddr *)&addr_client, sizeof(addr_client));
			}
			if(code == 4){
				strcpy(player2_Msg, buf);
				strcat(player2_Msg, " 4"); //Send Current Client's Code;
				//sprintf(player2_Msg, "%s%s%d", player2_Msg, " ", 4);
  				printf("[%d] send: %s from: %s\n", pid, player1_Msg, client_ip); 
				/* echo back */
				sendto(sfd, player1_Msg, len + 2, 0,  (struct sockaddr *)&addr_client, sizeof(addr_client));
			}
			//printf("[%d] received: %s from: %s\n", pid, buf, client_ip);  

		}
	}

	close(sfd);

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}
