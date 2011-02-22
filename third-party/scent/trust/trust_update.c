#include "trust_update.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include "scent_player_status.h"
int pua_debug = 0;
/*
int add_peer_to_list(struct peer_update_agent *pua, int uid, uint8_t *mac_addr,int tval);
int remove_peer_from_list(struct peer_update_agent *pua, uint8_t *mac_addr);
int send_update_req(struct peer_update_agent *pua);
int recv_update_res(struct peer_update_agent *pua);
*/



int add_peer_to_list(struct peer_update_agent *pua, int uid, uint8_t *mac_addr, int tval)
{
	int i =0;
	for(i=0;i<pua->neighbor_count; i++)
	{
		if(memcmp(pua->nlist[i].mac_addr,mac_addr, MAC_ADDR_LEN)==0) {
			printf("add_peer_to_list(): peer already exists mac_addr : %s : %s \n" ,mac_addr, macaddr_ntos(pua->nlist[i].mac_addr));
			return -1;
	 	}
	}
	//now  i = pua->neighbor_count;
	if(i >= MAX_PEERS)
	{
		printf("add_peer_to_list(): number of peers(%d) equals MAX_PEERS(%d) hence ignoring \n",i,MAX_PEERS );
		return -1;
	}
	pua->nlist[i].uid = uid;
	memcpy(pua->nlist[i].mac_addr,mac_addr,MAC_ADDR_LEN);
	pua->nlist[i].trust_val = tval;
	pua->neighbor_count++;
	printf("add_peer_to_list(): peer (%s) added to list, neighbor_count = %d \n",macaddr_ntos(mac_addr),pua->neighbor_count);
	return 1;
}


int remove_peer_from_list(struct peer_update_agent *pua, uint8_t *mac_addr)
{
	int i =0,j=0;
	for(i=0;i<pua->neighbor_count; i++)
	{
		if(memcmp(pua->nlist[i].mac_addr,mac_addr, MAC_ADDR_LEN)==0)
		{
			for(j=i;j<pua->neighbor_count; j++)
			{
				pua->nlist[j].uid =  pua->nlist[j+1].uid;
				memcpy(pua->nlist[j].mac_addr,pua->nlist[j+1].mac_addr,MAC_ADDR_LEN);
				pua->nlist[j].trust_val =  pua->nlist[j+1].trust_val;
			}
			pua->neighbor_count--;
			pua->nlist[j].uid = 0;
			memset(pua->nlist[j].mac_addr,0,MAC_ADDR_LEN);
			pua->nlist[j].trust_val = 0;
			break;
		}
	}
	if(i == pua->neighbor_count) 
	{
		printf("remove_peer_from_list(): no peer in the peer_update_list with mac_addr - %s \n",macaddr_ntos(mac_addr)  );
		return -1;
	}

	return 1;
}


int pua_get_index_by_macaddr(struct peer_update_agent *pua, uint8_t *mac_addr, int guess)
{

	if((guess >= 0)  && (guess < pua->neighbor_count))  {
		if (memcmp(pua->nlist[guess].mac_addr,mac_addr, MAC_ADDR_LEN)==0) return guess;
	 }

	int i =0;
	for(i=0;i<pua->neighbor_count; i++)
		if (memcmp(pua->nlist[i].mac_addr,mac_addr, MAC_ADDR_LEN)==0) return i;
	return -1;
}

int update_to_trust_agent(struct peer_update_agent *pua)
{
	if(pua->trustAgent)
	{		
		int i =0;
		struct timeval  update_time;
		gettimeofday(&update_time, NULL);
		for(i=0;i<pua->neighbor_count; i++)
			update_peerTrust(pua->trustAgent,pua->nlist[i].uid,pua->nlist[i].mac_addr,pua->nlist[i].trust_val,&update_time);
		if(pua->urgent_update) signal_urgent_update(pua->trustAgent);
		return i;
	}
	return 0;
}

int send_update_req(struct peer_update_agent *pua)

{
	int ret = 0;
	if((pua->tu_server_running) && (pua->tu_client_sock))
	{	
		int i  = 0;
		struct peer_update_msg *pumsg = (struct peer_update_msg *) malloc(sizeof (struct peer_update_msg));
		pumsg->type = PEER_TRUST_UPDATE_REQUEST;
		pumsg->uid = pua->uid;
		memcpy(pumsg->mac_addr, pua->mac_addr,MAC_ADDR_LEN);
		pumsg->neighborCount    = pua->neighbor_count;
		for (i=0; i < pua->neighbor_count; i++)
		{
			pumsg->nlist[i].uid = pua->nlist[i].uid;
			memcpy(pumsg->nlist[i].mac_addr,pua->nlist[i].mac_addr,MAC_ADDR_LEN);
			pumsg->nlist[i].trust_val = pua->nlist[i].trust_val;
		}
		
		ret = send(pua->tu_client_sock,(uint8_t *)pumsg,sizeof(struct peer_update_msg),MSG_NOSIGNAL);
		
	}
	return ret;
}
// sizeof (struct peer_update_msg) + 20  for 100 MAXPEEES ~ 829 
#define RU_RECVMSG_BUFSIZE_MAX  1000
int recv_update_res(struct peer_update_agent *pua)
{
	int ret = 0;
//	int bufsize = sizeof (struct peer_update_msg) + 20;
	uint8_t recvbuf[RU_RECVMSG_BUFSIZE_MAX];
	int recvlen = 0;
	if((pua->tu_server_running) && (pua->tu_client_sock))
	{	
		ret = recv(pua->tu_client_sock,recvbuf,RU_RECVMSG_BUFSIZE_MAX,0);
		if( ret < 0 )
			fprintf(stderr,"recv_update_res() : error - retval = %d, error (%d) : %s\n",ret,errno,strerror(errno));
		else if (ret > 0 )
		{
			if(ret < sizeof(struct peer_update_msg))  {
				fprintf(stderr, "recv_update_res() : read bytes :%d  < peer_update_msg size %d \n", ret,sizeof(struct peer_update_msg) );
				return 0;
			}
			struct peer_update_msg  *pumsg = (struct peer_update_msg *)recvbuf;
			if(pumsg->type != PEER_TRUST_UPDATE_RESPONSE) {
				fprintf(stderr,"recv_update_res() : read bytes :%d , peer_update_msg type (%d) is not PEER_UPDATE_RESPONSE, ignoring the msg \n", ret,pumsg->type );
				return 0;
			}
			
			if((pumsg->uid != pua->uid)  || (memcmp(pumsg->mac_addr,pua->mac_addr, MAC_ADDR_LEN)!=0)) 
			{
				fprintf(stderr,"recv_update_res() :this peer_update_msg is not to me ignring msg-mac(%s), my-mac(%s)\n",macaddr_ntos(pumsg->mac_addr),macaddr_ntos(pua->mac_addr));
				return 0;
			}
			int i = 0,index=0;
			pua->urgent_update = 0;
			for(i=0; i < pumsg->neighborCount; i++)
			{
				index = pua_get_index_by_macaddr(pua, pumsg->nlist[i].mac_addr,i);
				if(index >= 0)  {
						if( ((pua->nlist[index].trust_val >= NEIGHBOR_TRUST_THRESHHOLD) && (pumsg->nlist[i].trust_val < NEIGHBOR_TRUST_THRESHHOLD)) ||  ((pua->nlist[index].trust_val < NEIGHBOR_TRUST_THRESHHOLD) && (pumsg->nlist[i].trust_val >= NEIGHBOR_TRUST_THRESHHOLD)) )
							pua->urgent_update = 1;
						pua->nlist[index].trust_val = pumsg->nlist[i].trust_val;
						if(pua_debug) printf("recv_update_res() : updating  trust value for uid:%d - trust_value:%d \n", pua->nlist[index].uid,pua->nlist[index].trust_val );
				}
			}
			update_to_trust_agent(pua); 
		} else
			fprintf(stderr,"recv_update_res() : couldn't read - ret = %d, error (%d) : %s\n",ret,errno,strerror(errno));
	
	}
	return ret;

}

int is_scent_vplayer_stopped()
{
	if(scent_video_player_running) return 0;
	else {
		 fprintf(stderr,"is_scent_vplayer_running()  scent_video_player  stopped scent_video_player_running = %d \n",scent_video_player_running);
		 return 1;
	}
	return 0;
}


void send_recv_tu_msg(struct peer_update_agent *pua)
{
	int sentbytes = 0;
	int recvbytes = 0;
	int pumsglen = sizeof(struct peer_update_msg); 
	pua->tu_client_running =  1 ;
	
	while( (pua->tu_client_running) && (pua->tu_client_sock))
	{
		sentbytes = send_update_req(pua);
		if(sentbytes < pumsglen)
		{
			perror("send_recv_tu_msg(): write error");
			fprintf(stderr,"send_recv_tu_msg(): write error actual_len = %d, write_len = %d , errror(%d) : %s\n",pumsglen,sentbytes,errno,strerror(errno));
			if(errno == EPIPE) { fprintf(stderr,"send_recv_tu_msg(): EPIPE : Broken pipe: closing the socket\n");   break; }
			if(errno == ENOSPC) { fprintf(stderr,"send_recv_tu_msg(): ENOSPC : No space left on device :  closing the socket\n"); break; }
			if(sentbytes < 0)  {
				 fprintf(stderr,"send_recv_tu_msg():  error(%d) : %s  stoping the client connection sock(%d)\n",errno,strerror(errno),pua->tu_client_sock);
				 break;				 
			}
		} else 
		{
			if(is_scent_vplayer_stopped()) break;
			recvbytes = recv_update_res(pua);
			if((recvbytes == 0) || (recvbytes < 0))
			{ 
				fprintf(stderr,"send_recv_tu_msg(): read error actual_len = %d, read_len = %d , errror(%d) closing the client sock: %s\n",pumsglen,sentbytes,errno,strerror(errno));
				perror("send_recv_tu_msg(): read error");
				usleep(100);
				break;
			}
		}
		usleep(TU_UPDATE_INTERVAL);
		
		if(!pua->tu_server_running)	break;
		if(is_scent_vplayer_stopped()) break;
	}
	if(pua->tu_client_sock) close(pua->tu_client_sock);
		
}


void *start_trustUpdate_server(void *args)
{
	struct peer_update_agent *pua = (struct peer_update_agent *)args;

	printf("start_trustUpdate_server() :Starting new trustUpdateserver on port %d\n", TU_SERVER_PORT);
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TU_SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int on = 1;
	int trupdate_server_ready = 0;
	
	if(!trupdate_server_ready)
	{
	    if ((pua->tu_server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		fprintf(stderr,"start_trustUpdate_server() : socket creation failed due to error : %d - %s\n",errno,strerror(errno));
		pua->exit_func(SIGINT);
		return NULL;
		//sleep(2);
		//continue;
	   }
	   if (setsockopt(pua->tu_server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	   {
		perror("start_trustUpdate_server() : setsockopt failed for trustUpdate  server on SO_REUSEADDR");			            /* some error has occurred, etc */
 	   }
	   if (bind(pua->tu_server_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		char errbg[100];
		sprintf(errbg,"start_trustUpdate_server() : bind() failed for trustUpdate_server on %s:%d \n",inet_ntoa(addr.sin_addr),TU_SERVER_PORT);
		fprintf(stderr,"start_trustUpdate_server() : bind() failed for trustUpdate_server on %s:%d  due to error : %d - %s\n",inet_ntoa(addr.sin_addr),TU_SERVER_PORT,errno,strerror(errno));
		perror(errbg);
		close(pua->tu_server_sock);
		pua->exit_func(SIGINT);
		return NULL;
		//sleep(2);
		//continue;
	        //exit(1);
	   }
	   if (listen(pua->tu_server_sock, 10) == -1) {
		perror("start_trustUpdate_server() : listen()");
		close(pua->tu_server_sock);
		pua->exit_func(SIGINT);
		return NULL;
		//sleep(2);
		//continue;
	        //exit(1);
	   }
	   trupdate_server_ready = 1;   // Soerver socket ready now 
	}
	// Soerver socket ready now ..
	pua->tu_server_running = 1;
	struct sockaddr_in peer_addr;
	socklen_t sin_size = sizeof(peer_addr);
	while (pua->tu_server_running) {
		pua->tu_client_sock = accept(pua->tu_server_sock, (struct sockaddr *)&peer_addr, &sin_size);
		if (pua->tu_client_sock == -1) {
			perror("start_trustUpdate_server() : accept()");
			usleep(1000);
			continue;
		} else 
			printf("start_trustUpdate_server() : Got client - sock (%d)! starting the send_recv_tu_msg()  \n", pua->tu_client_sock);
		pua->tu_client_running = 1;
		send_recv_tu_msg(pua);
		if(pua->tu_client_sock) close(pua->tu_client_sock);
	}
	close(pua->tu_server_sock);
	printf("start_trustUpdate_server() : server exit\n");
	return NULL;
}

int start_peer_trust_update_thread(struct peer_update_agent *pua)
{
	int res = pthread_create(&(pua->thread_id),NULL,start_trustUpdate_server, (void *)pua);
	if(res < 0)  {
		perror ("start_peer_trust_update_thread(): Thread create error \n");
		//fprintf(stderr, "start_peer_trust_update_thread: Thread creation failed forstart_peer_trust_update_thread\n");
		pua->tu_server_running = 0;
		pua->thread_status = 0;
		return -1;
	} else  pua->thread_status = 1;
	
	printf("start_peer_trust_update_thread(): thread created successfully \n");
	
	return 1;
}

void init_peer_update_agent(struct peer_update_agent *pua,struct trust_agent  *tra, uint8_t nodeid,uint8_t *macaddr, void exitfunc(int))
{
	pua->trustAgent = tra;
	pua->uid = nodeid;
	memcpy(pua->mac_addr,macaddr,MAC_ADDR_LEN);
	pua->neighbor_count = 0;
	pua->tu_server_running = 0;
	pua->tu_server_sock = -1;
	pua->tu_client_sock = -1;
	pua->exit_func = exitfunc;
	pua->urgent_update = 0;
	//start_trustUpdate_server
	
	start_peer_trust_update_thread(pua);

}

void stop_peer_update_agent(struct peer_update_agent *pua)
{
	pua->tu_client_running = 0;
	pua->tu_server_running = 0;
	printf("stop_peer_update_agent() stopping internal peer trust update agent \n");
	usleep(TU_UPDATE_INTERVAL);
}

void stop_pua_client_conn(struct peer_update_agent *pua)
{
	pua->tu_client_running = 0;

}

void print_pumsg_values(struct peer_update_msg *pumsg)
{	
	int i = 0;
	printf("Peer Update Message \n");
	printf("=================== \n");
	printf("type         : %s\n", (pumsg->type == 15)?"REQUEST":"RESPONSE");
	printf("node-id      : %d\n",pumsg->uid);
	printf("mac-address  : %s\n", macaddr_ntos(pumsg->mac_addr));
	printf("neighborCount: %d\n",pumsg->neighborCount);
	printf("neighbor list \n");
	printf("uid----mac-address----trust-value\n");
	for(i=0; i < pumsg->neighborCount; i++)
		printf("%d - %s - %d\n",pumsg->nlist[i].uid,macaddr_ntos(pumsg->nlist[i].mac_addr),pumsg->nlist[i].trust_val);
	printf("-----------------------------------\n");
	printf("=================== \n");
	printf("\n");

}
