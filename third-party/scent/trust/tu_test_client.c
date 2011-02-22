#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "trust_update.h"
int is_client_running = 1;
//TU_SERVER_PORT
#include "scent_player_status.h"

int scent_video_player_running;
int start_tu_tcp_client()
{
	int client_sock;


	printf("start_tu_tcp_client() : Starting new start_trust_update_tcp_client on port %d\n", TU_SERVER_PORT);
	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sock < 0) {
		perror("socket()");
		return -1;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TU_SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int on = 1;
	if (setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	{
		perror("start_tu_tcp_client() : setsockopt failed for trustUpdate  server on SO_REUSEADDR");			            /* some error has occurred, etc */
	}

	
	if (connect(client_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		char errbg[100];
		sprintf(errbg,"start_tu_tcp_client() : connect() failed for start_tu_tcp_client on %s:%d ",inet_ntoa(addr.sin_addr),TU_SERVER_PORT);
		perror(errbg);
		close(client_sock);
	        return -1;
	}
	
	return client_sock;

}


static void  tu_test_client_sig_handler(int signo)
{
	if(signo == SIGBUS)
	{
		printf("tu_test_client_sig_handler() Signal SIGBUS received.. just ignoring \n");
		return;
	}
	if ((signo == SIGINT) || (signo == SIGTERM))
	{
		printf("tu_test_client_sig_handler() Signal %s received, exiting now \n", (signo==SIGINT)?"SIGINT":"SIGTERM");
		is_client_running = 0;
	}

}

int main(int argc, char **argv)
{
	uint8_t recv_buf[1000];
	int recvbytes =0,sendbytes=0, i=0;
	struct peer_update_msg *pumsg;	
	int client_sock =  start_tu_tcp_client();
	is_client_running = 1;
        signal(SIGINT,tu_test_client_sig_handler);
        signal(SIGBUS,tu_test_client_sig_handler);
	signal(SIGTERM,tu_test_client_sig_handler);

	if(client_sock)
	{
		while(client_sock && is_client_running)
		{
			recvbytes = recv(client_sock,recv_buf,1000,0);
			if(recvbytes > 0)
			{
				pumsg = (struct peer_update_msg *)recv_buf;
				printf("Received : \n");
			
				print_pumsg_values(pumsg);
				for(i=0; i<pumsg->neighborCount; i++)
					pumsg->nlist[i].trust_val = (i%2==0)?8:0;
				pumsg->type = PEER_TRUST_UPDATE_RESPONSE;
				printf("To be sent : \n");
				print_pumsg_values(pumsg);
				sendbytes = send(client_sock,(uint8_t *)pumsg,sizeof(struct peer_update_msg),0);
				printf("Sent %d bytes \n",sendbytes);
			}
			usleep(900);
		}
	}
	return 1;
}

