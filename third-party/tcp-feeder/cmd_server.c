/*
 * tcp-server.c
 *
 *  Created on: Apr 8, 2010
 *      Author: henkku
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>              // strlen
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>               /* read() */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

//#include "tcp-server.h"
#include "eventqueue.h"
#include "scent_player_status.h"


#define COMM_PORT 9998
int client_fd = 0;
int server_fd = 0;
int finished_movi_download = 0;
is_cmdserver_running = 0;
int play_status = 0; // 0 = paused, 1 = play called
char failed_sendmsg[200];
int  failed_sendmsg_len = 0;

void start_forced_play();

int scent_video_player_running = 0;

void signal_player_start()
{
	scent_video_player_running  = 1;
}

void signal_player_stop()
{
	scent_video_player_running = 0;
}

void *start_cmd_server(void *args)
{

	fprintf(stderr,"start_cmd_server(): tarting new CTRL server on port %d\n", COMM_PORT);
	
	int url_fd = 0;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(COMM_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int on = 1;
	int client_num = 0 ;
	int cmd_server_ready = 0;
 	int localpid = getpid();	
	if(!cmd_server_ready) {
	
	   if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		kill(localpid,SIGINT);
		return NULL;
		//sleep(2);
		//continue ;
	   }

	  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	  {
		perror("setsockopt failed for tcp_control server on SO_REUSEADDR");			            /* some error has occurred, etc */
	  }
	
	  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		char errbg[100];
		sprintf(errbg,"bind() failed for tcp_control server on %s:%d ",inet_ntoa(addr.sin_addr),COMM_PORT);
		perror(errbg);
		fprintf(stderr,"bind() failed for tcp_control server on %s:%d, failed with errno %d : error : %s , calling eventqueue_shutdown() \n",inet_ntoa(addr.sin_addr),COMM_PORT,errno,strerror(errno));
		fprintf(stderr,"start_cmd_server() : calling kill -2 on pid %d\n",localpid);
		close(server_fd);
		kill(localpid,SIGINT);
	        return NULL;
		//exit(1);
		//sleep(2);
		//continue;
	  }
	  if (listen(server_fd, 10) == -1) {
		perror("listen()");
		fprintf(stderr,"start_cmd_server() : calling kill -2 on pid %d\n",localpid);
		close(server_fd);
		kill(localpid,SIGINT);
	        return NULL;
		//sleep(2);
		//continue;
	        //exit(1);
	  }
	  cmd_server_ready = 1;
	} //while socket loop
	printf("start_cmd_server() server sock(%d) ready , and pid = %d \n",server_fd,localpid );
	 // socket server ready
	memset(failed_sendmsg,0,failed_sendmsg_len);
	is_cmdserver_running = 1;
	while (is_cmdserver_running) {
		struct sockaddr_in peer_addr;
		socklen_t sin_size = sizeof(peer_addr);
		if(client_fd) close(client_fd);
		url_fd = accept(server_fd, (struct sockaddr *)&peer_addr, &sin_size);
		if (url_fd == -1) {
			perror("accept()");
			continue;
		}
		client_num++;
		const char *suff=(client_num ==1 )?"st":((client_num == 2)?"nd":((client_num ==3)?"rd":"th"));
		fprintf(stderr,"start_cmd_server(): got client connection %d%s time\n",client_num,suff);
	/*	if(client_fd)  {
			fprintf(stderr,"start_cmd_server() got new client connection.. hence closing the old client fd %d \n",client_fd);
			close(client_fd);
		} */
		client_fd = url_fd;
		if(failed_sendmsg_len > 0) { 
			int sentbytes = send_to_app(failed_sendmsg,failed_sendmsg_len);
			fprintf(stderr,"start_cmd_server() :  msg re-sent : %s  of bytes %d\n",failed_sendmsg,failed_sendmsg_len,sentbytes);  
			memset(failed_sendmsg,0,failed_sendmsg_len);
			failed_sendmsg_len = 0;
		}
		signal_player_start();
		if(client_num > 1) start_forced_play();
		eventqueue_run();
		signal_player_stop();
	}
	close(client_fd);
	close(server_fd);
	fprintf(stderr,"start_cmd_server(): tcp control server exit\n");
	return NULL;
}


/*
 * TODO: Doesn't handle properly the case of a client disconencting
 * and then reconnecting
 */
int send_to_app(char *buff, int len)
{
	if(!client_fd)
	{
		memcpy(failed_sendmsg,buff,len);
		fprintf(stderr, "send_to_app() client_fd is closed so Wrote 0 bytes out of %d\n", len);
		failed_sendmsg_len = len;
		eventqueue_shutdown();
		return -1;
	}

	int ret = send(client_fd, buff, len, MSG_NOSIGNAL);
	//fprintf(stderr, "Sending to video player bytes %d of total so far: %d \n", len, seg);
	if(ret <0){
		perror("send_to_app() Sending of msg to application failed\n");
		fprintf(stderr,"send_to_app() Sending of msg to application failed with errno %d : error : %s , calling eventqueue_shutdown() \n",errno,strerror(errno));
		close(client_fd);
		memcpy(failed_sendmsg,buff,len);
		failed_sendmsg_len = len;
		eventqueue_shutdown();
		return -1; 
	} else if (ret == 0)
		fprintf(stderr, "send_to_app() Wrote 0 bytes out of %d\n", len);
	return ret;
}

void cmd_server_finish()
{
	is_cmdserver_running = 0;
	eventqueue_shutdown();
	// close(url_fd);
	// close(server_fd);
	sleep(2);
}

pthread_t server_thread;
void start_play()
{
  if(!play_status) 
  {
 	 //finished_movi_download = 0;
  	char *ptr = "start\n";
  	eventqueue_add(ptr, sizeof("start\n"));
  	fprintf(stderr,"start_play() called \n");
  	play_status = 1;
  }  // else printf("start_play() : Already playing \n");
}
void pause_play()
{
 if(play_status)
 {
  char *ptr = "pause\n";
  eventqueue_add(ptr, sizeof("pause\n"));
  fprintf(stderr,"pause_play() called \n");
  play_status = 0;
 } else printf("pause_play() : Already paused \n");
}

void start_forced_play()
{
  	char *ptr = "start\n";
  	eventqueue_add(ptr, sizeof("start\n"));
  	fprintf(stderr,"start_forced_play() called sending a start \n");
  	play_status = 1;
}

void stop_play()
{
   fprintf(stderr,"stop_play() called \n");
   //finished_movi_download = 1;
   char *ptr = "stop\n";
  eventqueue_add(ptr, sizeof("stop\n"));
}

void interrupt_stop_play()
{

   fprintf(stderr,"interrupt_stop_play() called \n");
   char *ptr = "interrupt_stop\n";

  // This will close the player remote, thus we need to inform the other local socket connections
  signal_player_stop();
	
   int len =  strlen(ptr);

   int ret = send(client_fd, ptr, len, MSG_NOSIGNAL);  
   if(ret <0){
		perror("interrupt_stop_play() Sending of msg to application failed\n");
		fprintf(stderr,"interrupt_stop_play() Sending of msg to application failed with errno %d : error : %s , calling eventqueue_shutdown() \n",errno,strerror(errno));
		close(client_fd);
		eventqueue_shutdown();
		return -1; 
	} else if (ret == 0)
		fprintf(stderr, "send_to_app() Wrote 0 bytes out of %d\n", len);
  //eventqueue_add(ptr, sizeof("interrupt_stop\n"));
  //char *sptr = "start\n";
  //eventqueue_add(sptr, sizeof("start\n"));
  // This will close the player remote, thus we need to inform the other local socket connections
  //signal_player_stop();
}

void continue_play()
{
   fprintf(stderr,"continue_play() called \n");
   char *ptr = "continue\n";
  eventqueue_add(ptr, sizeof("continue\n"));
}

int get_play_status()
{
	return play_status;
}

void set_movi_finish_download()
{ 
	if(!finished_movi_download)  {
		fprintf(stderr,"movi download completed, let's set movi_finish_download flag \n");
		finished_movi_download = 1;
	}
}

void unset_movi_finish_download()
{
	finished_movi_download = 0;	
}
int is_movi_download_finished()
{
	return finished_movi_download;
}

int init_cmd_server()
{
	//printf("starting tcp server\n");
	//pthread_mutex_init(&read_lock, NULL);
	//pthread_cond_init(&can_read, NULL);

	eventqueue_init(send_to_app, 1);

	pthread_create(&server_thread, NULL, start_cmd_server, NULL);
	//pthread_create(&data_server_thread, NULL, start_data_server, NULL);
	//start_data_server();
	return 0;
}
#ifdef TEST_TCPSERVER

int main(void)
{

  int i;

  init_tcp_server();
  sleep(5);
  for (i=0; i<10; i++){
    start_player();
    sleep(1);
    pause_player();
    sleep(1);
  }
  printf("\nend\n");
  tcp_serv_finish();
  //pthread_join(*sthread, NULL);

  return 0;
}
#endif
