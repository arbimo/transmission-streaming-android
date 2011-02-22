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
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp-server.h"

#define DATA_PORT 8889
pthread_t data_server_thread;

int client = 0;
int close_server = 0;
int client_connected = 0;

int data_server_fd = -1;
int data_sock = -1;

uint64_t write_total_global = 0;
uint64_t read_total_global = 0;
double this_movi_rate = 100000.0; // in kbits/sec
#define PAUSE_PKT_LIMIT 3

void player_add_write_bytes(int wbytes)
{
	write_total_global += wbytes;
}	

uint64_t player_get_write_bytes()
{
	return write_total_global;
}

int is_tcp_server_running()
{
	return (!close_server);
}

void player_add_read_bytes(int wbytes)
{
	read_total_global += wbytes;
}	

void player_reset_read_bytes()
{
	        read_total_global = 0;
}

uint64_t player_get_read_bytes()
{
	return read_total_global;
}

void player_set_movi_rate(double rate)
{
	this_movi_rate = rate;
}

int is_player_ready()
{
	return client_connected;
}

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },  
	{"jpg", "image/jpeg"}, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" }, 
	{"mp4", "video/mp4" },  
	{"wmv", "video/wmv" },  
	{0,0} };

//#define BUFSIZE 8096
#define BUFSIZE 1536
#define PAUSED_STATE_BUFFER_LIMIT 20000
static int seg = 0;
int send_to_player(char *buff, int len)
{
 
  while (!client_connected) {
    fprintf(stderr, "Waiting for Video player\n");
    sleep(1);
    if (close_server)
      return 0;
  }
  seg += len;
  fprintf(stderr, "Sending to video player bytes %d of total so far: %d \n", len, seg);
  return send(data_sock, buff, len, 0); 
}

int global_signal_interrupt = 0;

void handle_req(int fd, int client_num)
{
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	static char buffer[BUFSIZE+1]; /* static so zero filled */
	int continue_play_sent = 0;
	global_signal_interrupt =  0;
	ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) {	/* read failure stop now */
	  fprintf(stderr,"failed to read browser request\n");
	}
	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		buffer[ret]=0;		/* terminate the buffer */
	else buffer[0]=0;

	for(i=0;i<ret;i++)	/* remove CF and LF characters */
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i]='*';
	fprintf(stderr,"request %s",buffer);

	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) )
		fprintf(stderr,"Only simple GET operation supported request:%s fd:%d",buffer,fd);

	for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}

	for(j=0;j<i-1;j++) 	/* check for illegal parent directory use .. */
		if(buffer[j] == '.' && buffer[j+1] == '.')
			fprintf(stderr,"Parent directory (..) path names not supported %s:%d",buffer,fd);

	if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) /* convert no filename to index file */
		(void)strcpy(buffer,"GET /index.html");

	/* work out the file type and check we support it */
	buflen=strlen(buffer);
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
		}
	}
	if (fstr == 0) 
		fprintf(stderr,"file extension type not supported");

	(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "video/mp4");
	//(void)write(fd,buffer,strlen(buffer));
	send(fd,buffer,strlen(buffer),MSG_NOSIGNAL);
	/* Enable movi_client to write to the socket */
	client_connected = 1;
#if 0
}
#endif
	client_connected = 1;

	int read_total = 0;
	int max_read = 0;
	int read_now = 0;
	uint32_t typical_usleep =((double)(BUFSIZE*8)/(this_movi_rate))*1000000;

	while(( file_fd = open("/sdcard/1009.mp4",O_RDONLY)) == -1){ /* open the file for reading */
		 
		fprintf(stderr, "failed to open file, waiting for the file %d\n", errno);
		//exit(1);
		sleep(1);
	}
	fprintf(stderr, "\ntypical_usleep = %d micro seconds i, this_movi_rate = %d, BUFSIZE =%d \n",typical_usleep,this_movi_rate,BUFSIZE);
	/* send file in 8KB block - last block may be smaller */
	int nodata_counter = 0;
	int writelen = 0;
	int dataAvail = 0;
	int pauseData = 0;
	unset_movi_finish_download();
	while (ret > 0) {
		dataAvail = player_get_write_bytes();
		read_total = player_get_read_bytes();
		max_read = dataAvail - read_total;
		if(global_signal_interrupt > 0) break;
		if (max_read <= 0){
		  
			//fprintf(stderr, "Can't read anymore!\n");
			if (is_movi_download_finished())
				break;
			sleep(1);
			nodata_counter++;
			if(nodata_counter == PAUSE_PKT_LIMIT)  { pause_play(); pauseData = 0; }
			fprintf(stderr, "handle_req(): No data to read! Data Avail = %d, Data Read = %d. No-Data_Counter = %d\n",dataAvail,read_total,nodata_counter);
			continue;
		} else 
		{
			if(!get_play_status()) pauseData += max_read;
			if (max_read > BUFSIZE)
			read_now = BUFSIZE;
			else {
		  		fprintf(stderr, "Can read only %d bytes\n", max_read);
		     		read_now = max_read;
			}
		}
		ret = read(file_fd, buffer, read_now); 
		if (ret > 0) {
			//writelen = write(fd,buffer,ret);
			writelen = send(fd,buffer,ret,MSG_NOSIGNAL); 
			if(writelen < ret )
			{
				perror("handle_req(): write error");
				fprintf(stderr,"handle_req(): write error read = %d, write = %d \n",ret,writelen );
				if(errno == EPIPE) { fprintf(stderr,"handle_req(): EPIPE : Broken pipe: closing the socket\n");  interrupt_stop_play(); break; }
				if(errno == ENOSPC) { fprintf(stderr,"handle_req(): ENOSPC : No space left on device :  closing the socket\n"); interrupt_stop_play(); break; }
			}
			/*
			if(read_total > 300000)
			{
				if((client_num > 1) && (!continue_play_sent)) { continue_play(); continue_play_sent = 1; }
			} */

			if((read_total > 200000) && ( max_read < 400000))  { 
				usleep(((double)(ret*8)/(this_movi_rate))*1000000/2);
			}

			if((nodata_counter >= PAUSE_PKT_LIMIT) && (pauseData > PAUSED_STATE_BUFFER_LIMIT))  { start_play(); nodata_counter = 0; pauseData = 0; }
			//read_total +=ret;
			player_add_read_bytes(ret);
		} else fprintf(stderr,"handle_req(): coundn't read ret = %d , error: %d - %s\n", ret,errno,strerror(errno));
		if(global_signal_interrupt > 0) break;
	}
	//Playing finished.. stopping the player
	if(!(ret >  0 )) stop_play();  // if ret < 0, then we already sent a interrupt_stop_play() ... no need for a stop_play() again ...
	sleep(3);	/* to allow socket to drain */
	if(file_fd) close(file_fd);
	if(fd) close(fd);
	fprintf(stderr,"handle_req(): exiting handle_req() \n");
}
static void TsSigHandler(int signo)
{
	if(signo == SIGUSR1)
	{	fprintf(stderr,"TsSigHandler(): SIGUSR1 signal received \n");
		interrupt_stop_play();
		global_signal_interrupt = 1;
	}
}

/*
 * start_data_server
 * @args : contains pointer to function which allows reading blocks of data.
 * The function has two arguments: char buffer into which data is written and int max_data_length.
 * The function returns amount of actual data written into the buffer.
 */
void *start_data_server(void *args)
{
#if 0
		int (* read_data)(char *, int) = (int (*)(char *, int)) args;
#endif
		printf("Starting DATA server on port 8889\n");
	 	int typical_usleep =((double)(BUFSIZE*8)/(this_movi_rate))*1000000;
		fprintf(stderr, "\ntypical_usleep = %d micro seconds \n",typical_usleep);
		signal(SIGUSR1,TsSigHandler);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(DATA_PORT);
		addr.sin_addr.s_addr = INADDR_ANY;
		int on = 1;
		int localpid = getpid();

		int data_server_ready = 0;
		if(!data_server_ready) {
		  if ((data_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket()");
			kill(localpid,SIGINT);
			return NULL;
			//sleep(2);
			//continue ;
		  }
		  if (setsockopt(data_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
		  {
			perror("setsockopt failed for tcp_data_server on SO_REUSEADDR");			            /* some error has occurred, etc */
		  }
		  if (bind(data_server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
			char errbg[100];
			sprintf(errbg,"bind() failed for tcp_data_server on %s:%d ",inet_ntoa(addr.sin_addr),DATA_PORT);
		        perror(errbg);
			fprintf(stderr,"start_data_server() : bind() failed for tcp_data_server on %s:%d  due to error : %d - %s\n",inet_ntoa(addr.sin_addr),DATA_PORT,errno,strerror(errno));
			fprintf(stderr,"start_data_server() : calling kill -2 on pid %d\n",localpid);
			kill(localpid,SIGINT);
			return NULL;
			//sleep(2);
			//continue ;
		  }
		  if (listen(data_server_fd, 10) == -1) {
			perror("listen()");
			kill(localpid,SIGINT);
			return NULL;
			//sleep(2);
			//continue ;
			//exit(1);
		  }
		  data_server_ready = 1;
		}
		int client_num = 0;
		while (!close_server) {
			struct sockaddr_in peer_addr;
			unsigned int sin_size = sizeof(peer_addr);
			data_sock = accept(data_server_fd, (struct sockaddr *)&peer_addr, &sin_size);
			if (data_sock == -1) {
				perror("accept()");
				break;

			}
			client_num++;
			const char *suff=(client_num ==1 )?"st":((client_num == 2)?"nd":((client_num ==3)?"rd":"th"));
			fprintf(stderr,"start_data_server() : client connected %d%s times \n", client_num,suff); 
			start_forced_play();
			if( client_num > 1)    {
				fprintf(stderr,"start_data_server() :client reconnected so reset the reader to start from teh beginning \n");
				player_reset_read_bytes();
			}
			handle_req(data_sock,client_num);
			eventqueue_shutdown(); // to close control client socket

		}
		if(data_sock) close(data_sock);
		close(data_server_fd);
		printf("tcp data server exit\n");
		return NULL;
}

void tcp_serv_finish()
{
	close_server = 1;
	sleep(2);
	//close(data_sock);
	//close(data_server_fd);
}

int init_tcp_serv()
{
	pthread_create(&data_server_thread, NULL, start_data_server, NULL);
	return 0;
}

