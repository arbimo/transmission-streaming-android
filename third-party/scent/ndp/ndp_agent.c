
#include <sys/ipc.h>
#include <sys/stat.h>
//#include <sys/file.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <err.h>
#include <math.h>
#include <pthread.h>
//#include "wifi.h"
#include "ndp_agent.h"
#include "ndp_msg.h"
#include "../utils/netutils_common.h"
//int wifi_start_supplicant();
//int wifi_command(const char *command, char *reply, size_t *reply_len);


/** Neighbor Discovery Protocol : NDP  */
#define NDPEXIT(x) { fprintf(stderr,"exiting with %d",x); exit(x); }
//void *start_wifi_scan(void *arg);

void *ndp_send_recv(void *arg);

int get_ndp_send_sock() {
	int     reuse=1;
        int int_op = 48,s;
	s =socket(AF_INET, SOCK_DGRAM, 0);
        if (setsockopt(s, IPPROTO_IP, IP_TOS, &int_op, sizeof(int_op)) == -1)
        {
                perror("get_ndp_send_sock() : setsockopt-IP_TOS: ");
                NDPEXIT(0);
        }

        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse)) == -1)
        {
                perror("get_ndp_send_sock() : setsockopt-SO_REUSEADDR: ");
                NDPEXIT(0);
        }
	return s;
}

int get_ndp_recv_sock(uint32_t bind_ip, int bind_port)
{
	//struct ifreq ifr;
	int recvsock= socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (recvsock < 0) {
                perror("init_ndp_agent(): socket");
                return -1;
        } 
	int reuse=1,ret_value,on=1;
	struct sockaddr_in my_addr;
	memset(&my_addr,0, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = bind_ip; // my local address of the wifi interface
        my_addr.sin_port = htons(bind_port);
 
       if (setsockopt(recvsock, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse)) == -1) {
                perror("get_recv_sock(): setsockopt");
                return -1;
        }

        ret_value= setsockopt(recvsock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int));
        if (ret_value < 0) {
                perror("get_recv_sock(): SO_BROADCAST Failed ");
                return -1;
        }
/*
        memset(&ifr,0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
        ret_value=setsockopt( recvsock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
        if(ret_value < 0) {
                fprintf(stderr,"get_recv_sock() : SO_BINDTODEVICE failed for %s device",ifr.ifr_name);
                perror("get_recv_sock() : ");
                return -1;
        }
*/
        ret_value= bind(recvsock, (struct sockaddr*) &my_addr,  sizeof(struct sockaddr));
        if (ret_value < 0) {
                perror("get_recv_sock(): Socket Bind Failed ");
                return -1;
        }
	struct in_addr baddr = { bind_ip };
	printf("get_recv_sock(): returning sock(%d) for %s:%d\n", recvsock,inet_ntoa(baddr),bind_port);
	return recvsock;

}


int ndp_thread_start(struct ndp_agent *ndpAgent)
{
	if(!ndpAgent) return -1;
	ndpAgent->thread_running = 1;
	printf("ndp_thread_start() : creating ndp_thread \n");
	int res = pthread_create(&(ndpAgent->thread_id),NULL,ndpAgent->thread_runner, (void *)ndpAgent);
	if(res < 0)  {
		perror ("ndp_thread_start(): Thread create error \n");
		fprintf(stderr, "ndp_thread_start(): Thread creation failed for ndp_thread\n");
		ndpAgent->thread_running = 0;
		ndpAgent->thread_status = 0;
		return -1;
	} else ndpAgent->thread_status = 1;
	//pthread_join(ndpAgent->thread_id,NULL);
	printf("ndp_thread_start():  ndp_thread created successfully \n");
	//pthread_join(ndpAgent->thread_id,NULL);
	return 1;
}


void stop_ndp_agent(struct ndp_agent *ndpAgent)
{
	if(ndpAgent->thread_status) printf("stop_ndp_agent(): Stopping ndp_thread \n");
	ndpAgent->thread_running = 0;
	usleep(10);
	//pthread_detach(mct->id);
}

void wait_for_ndpthread_exit(struct ndp_agent *ndpAgent)
{
	// extra caoution because  pthread_join does not work well on android

	printf("wait_for_ndpthread_exit() : about to check the ndp thread \n"); 
	do {
		usleep(1000000);
	} while (ndpAgent->thread_status) ;
	
	printf("wait_for_ndpthread_exit() : thread_status = STOPPED, Hence  exiting  wait loop now \n"); 
}

void init_ndp_agent(struct ndp_agent *ndpAgent,uint8_t uid, const char *ifname, void  update_callback(void *,uint8_t,const uint8_t *,uint8_t,uint32_t), void *update_callback_arg)
{
	ndpAgent->uid = uid;
	ndpAgent->req_count = 0;
	u_int8_t macaddr[MAC_ADDR_LEN];
	get_if_macaddr(macaddr,ifname);
	memcpy(ndpAgent->mac_addr,macaddr, MAC_ADDR_LEN);
	get_local_essid(&(ndpAgent->essid[0]));
	strcpy(ndpAgent->ifname,ifname);
	ndpAgent->freq = get_local_wifi_freq(ifname);
        ndpAgent->curr_rssi = get_local_wifi_rssi(ifname);
	gettimeofday(&(ndpAgent->ref_time),NULL);

	// Set the NDP_RESPONSE Recv socket
	uint32_t ipaddr = get_if_ipaddr(ifname);
	ndpAgent->ip_addr = ipaddr;
	int recvsock = get_ndp_recv_sock(ipaddr,NDP_PORT);
	if (recvsock < 0) {
                perror("init_ndp_agent(): ndp_res_recv socket");
                NDPEXIT(-1);
        } 
	ndpAgent->res_recv_sock = recvsock;
	
	// Set the NDP_REQUEST BROADCAST Recv socket
	ipaddr = get_if_bcastaddr(ifname);
	recvsock= get_ndp_recv_sock(ipaddr,NDP_PORT);
	if (recvsock < 0) {
                perror("init_ndp_agent(): ndp_req_recv socket");
                NDPEXIT(-1);
        } 
	ndpAgent->req_recv_sock = recvsock;
	
	int sendsock = get_ndp_send_sock(); 
	if (sendsock < 0) {
                perror("init_ndp_agent(): ndp_req_send socket");
                NDPEXIT(-1);
        } 
	ndpAgent->req_send_sock = sendsock;
	
	sendsock = get_ndp_send_sock(); 
	if (sendsock < 0) {
                perror("init_ndp_agent(): ndp_res_send socket");
                NDPEXIT(-1);
        } 
	ndpAgent->res_send_sock = sendsock;
	
	ndpAgent->bcast_addr = ipaddr; 

	ndpAgent->thread_runner = ndp_send_recv;
	//ndpAgent->thread_runner = start_wifi_scan;
	ndpAgent->update_neighbor_callback = update_callback;
	ndpAgent->update_arg = update_callback_arg;
	printf("init_ndp_agent() : ndp_agent inilized, ndpAgent->res_recv_sock = %d,ndpAgent->req_recv_sock = %d, ether : %s \n",ndpAgent->res_recv_sock,ndpAgent->req_recv_sock,macaddr_ntos(ndpAgent->mac_addr));

	ndp_thread_start(ndpAgent);
}

char *get_ndp_req_str(struct ndp_agent *ndpAgent,struct ndp_request *ndpreq)
{
	char *req_str = &(ndpAgent->ndp_str[0]);
	struct in_addr ipaddr = { ndpreq->ip_addr };
	sprintf(req_str,"ndp_request (type=%d, uid=%d, seq=%d, ip=%s) ", ndpreq->type,ndpreq->uid,ndpreq->seq_count,inet_ntoa(ipaddr));
	return req_str;
}

// ref_time
char *get_ndp_res_str(struct ndp_agent *ndpAgent,struct ndp_resp *ndpres)
{
	char *res_str = &(ndpAgent->ndp_str[0]);
	struct in_addr ipaddr = { ndpres->ip_addr };
	sprintf(res_str,"ndp_response (type=%d, uid=%d, freq=%d, rssi=%d, mac-addr=%s, essid=%s, seq=%d, ip=%s, last_sense=%u) ", ndpres->type,ndpres->uid,ndpres->freq,ndpres->rssi,macaddr_ntos(ndpres->mac_addr),(char *)(ndpres->essid),ndpres->seq_count,inet_ntoa(ipaddr),get_timediff_usec(&(ndpres->last_sense),&(ndpAgent->ref_time)));
	return res_str;
}

void close_ndp_socks(struct ndp_agent *ndpAgent)
{
	if(ndpAgent->res_recv_sock) close(ndpAgent->res_recv_sock);
	if(ndpAgent->req_recv_sock) close(ndpAgent->req_recv_sock);
	if(ndpAgent->res_send_sock) close(ndpAgent->res_send_sock);
	if(ndpAgent->req_send_sock) close(ndpAgent->req_send_sock);
	//if(!is_ndp_stopped()) stop_ndp_agent(ndpAgent);
	
}


inline int is_ndp_stopped(struct ndp_agent *ndpAgent)
{
	return !(ndpAgent->thread_status);
}

int send_ndp_req_broadcast(struct ndp_agent *ndpAgent) {

        struct sockaddr_in dst_addr;
	struct ndp_request *ndpreq;
	int ret_value=1, on=1;

	dst_addr.sin_family = AF_INET;
	//dst_addr.sin_addr.s_addr = 0xFF03A8C0;
	dst_addr.sin_addr.s_addr =  ndpAgent->bcast_addr; // get_ifbcast_addr(ifname);
	dst_addr.sin_port = htons(NDP_PORT);
	ndpAgent->req_count++;
 	ndpreq = (struct ndp_request *)malloc(sizeof(struct ndp_request));
	// Form ndp_req packet	
	ndpreq->type = NDP_REQUEST;
	ndpreq->uid = ndpAgent->uid;
	ndpreq->seq_count = ndpAgent->req_count;
	ndpreq->ip_addr = ndpAgent->ip_addr;

	int send_sock = ndpAgent->req_send_sock;
	ret_value= setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int));
        if (ret_value < 0) {
                perror("send_ndp_req_broadcast(): SO_BROADCAST Failed ");
                return -1;
        }
	ret_value = sendto(send_sock, ndpreq, sizeof(struct ndp_request ), MSG_NOSIGNAL, (struct sockaddr *) &dst_addr, sizeof (dst_addr));
	//printf("SEND PMS RESP %d\n", idls_resp_pkt->cf);	
        if(ret_value == -1) {
                perror("send_ndp_req_broadcast() : sendto");
                //NDPEXIT(-1);
		fprintf(stderr, "send_ndp_req_broadcast() error in sending ndp_req \n");
        }
	free(ndpreq);
	//close(send_sock);
        return ret_value;

}

int send_ndp_res(struct ndp_agent *ndpAgent, const struct ndp_request *ndpreq) {
	
	struct sockaddr_in dst_addr;
	struct ndp_resp  *ndpres;
	int send_sock = 0;
	int ret_value=1;
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_addr.s_addr = ndpreq->ip_addr;
	dst_addr.sin_port = htons(NDP_PORT);
	ndpres = (struct ndp_resp *)malloc(sizeof(struct ndp_resp));

	ndpres->type = NDP_RESPONSE;
	ndpres->uid = ndpAgent->uid;
	ndpres->freq =  ndpAgent->freq;
	ndpres->rssi =  ndpAgent->curr_rssi;
	memcpy(ndpres->mac_addr,ndpAgent->mac_addr,MAC_ADDR_LEN);
	memcpy(ndpres->essid,ndpAgent->essid,IEEE80211_ADDR_LEN);
	ndpres->seq_count = ndpreq->seq_count;
	ndpres->ip_addr = ndpAgent->ip_addr;
	gettimeofday( &(ndpres->last_sense), NULL);
	//ndpres->status = ndpAgent->mc->FSM.status;
	send_sock = ndpAgent->res_send_sock;
	ret_value = sendto(send_sock, ndpres, sizeof(struct ndp_resp), MSG_NOSIGNAL, (struct sockaddr *) &dst_addr, sizeof (dst_addr));
	if(ret_value > 0)  { } //printf("send_ndp_res() : %s \n", get_ndp_res_str(ndpAgent,ndpres));
	else perror("send_ndp_res() : ");
	return ret_value;
}

void update_ndp_agent(struct ndp_agent *ndpAgent)
{
	// TBD
	ndpAgent->freq = get_local_wifi_freq(ndpAgent->ifname);
        ndpAgent->curr_rssi = get_local_wifi_rssi(ndpAgent->ifname);

}

int is_valid_ndpreq(struct ndp_agent *ndpAgent,const 	struct ndp_request *ndpreq)
{
	int retval = 1;
	//ndpreq->uid check whether uid is valid/not block
	// TBD

	return retval;
}

int is_valid_ndpres(struct ndp_agent *ndpAgent, const struct ndp_request *ndpres)
{
	int retval = 1;
	//ndpres->uid check whether uid is valid/not block
	// TBD
	
	return retval;
}

int process_ndp_req(struct ndp_agent *ndpAgent,const struct ndp_request *ndpreq)
{
	// Ignore the self request ..
	if((ndpreq->uid == ndpAgent->uid) && (ndpreq->ip_addr == ndpAgent->ip_addr))
	return 0;
	
	update_ndp_agent(ndpAgent);
	if(is_valid_ndpreq(ndpAgent,ndpreq) > 0) send_ndp_res(ndpAgent,ndpreq);
	else   { fprintf(stderr,"process_ndp_req() wrong / blocked uid %d \n",ndpreq->uid);  return -1; }
	return 1;
}

void process_ndp_res(struct ndp_agent *ndpAgent, const struct ndp_resp *ndpres )
{
	struct timeval recv_time;
	gettimeofday(&recv_time,NULL);
	uint32_t rx_ref_usec = get_timediff_usec(&recv_time,&(ndpAgent->ref_time));
	if(ndpres->seq_count != ndpAgent->req_count)
	{
		fprintf(stderr,"process_ndp_res() : Invalid ndp response req_count=%d, ndpres->seq_count = %d \n", ndpAgent->req_count,ndpres->seq_count);
		return ; // ignore this ndp response
	} 

	if((ndpres->uid == ndpAgent->uid) && (memcmp(ndpres->mac_addr,ndpAgent->mac_addr, MAC_ADDR_LEN)==0))
	{
		fprintf(stderr,"process_ndp_res() : my own ndp response ignoring ..\n");
		return ;
	}
	
	//return update_neighbor(&(ndpAgent->mc->neighborList),ndpres->rssi,ndpres->mac_addr,ndpres->uid,rx_ref_usec);	
	ndpAgent->update_neighbor_callback(ndpAgent->update_arg,ndpres->rssi,&ndpres->mac_addr[0],ndpres->uid,rx_ref_usec);
}




void *ndp_send_recv(void *arg)
{
	struct ndp_agent *ndpagent = (struct ndp_agent *)arg;
	ndpagent->thread_running = 1; // Thread active
	int res_recv_sock = ndpagent->res_recv_sock;
	int req_recv_sock = ndpagent->req_recv_sock;
	int nfds_max = (req_recv_sock > res_recv_sock)?(req_recv_sock+1):(res_recv_sock+1);
	int recv_sock = 0;
	int resmsg_avail = 0;
	int reqmsg_avail = 0, i=0;
	struct timeval curr_time,last_sent,totv;
	struct timespec tots;
	int ret = 0,numbytes=0,status; 
	uint32_t diff_usec;
	fd_set reads;
	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof(struct sockaddr);
	char *recvbuf = (char *)malloc (1500);
	struct ndp_request  *ndp_req;
	struct ndp_resp	    *ndp_res;
	sigset_t	emptyset, blockset;	
	
	// NDP_RECV_TIMEOUT is in milli seconds, so converting it into time_val :  sec + micro seconds
	int timeout_sec = NDP_RECV_TIMEOUT/1000;
	int timeout_usec = (NDP_RECV_TIMEOUT%1000)*1000;

	gettimeofday(&curr_time,NULL);
	update_ndp_agent(ndpagent);
	ret=send_ndp_req_broadcast(ndpagent);
	if(ret < 0 )
	{
		fprintf(stderr,"ndp_send_recv(): send_ndp_req_broadcast failed.. will try once more \n");
		ret=send_ndp_req_broadcast(ndpagent);
		if(ret < 0) fprintf(stderr,"ndp_send_recv(): send_ndp_req_broadcast failed again.. will giveup now a.. and try later .. \n");
		
	}
	//sigemptyset(&blockset);
	//sigaddset(&blockset,SIGINT);
	//sigaddset(&blockset,SIGTERM);
	printf("ndp_send_recv(): starting teh whiel loop \n");

	gettimeofday(&last_sent,NULL);
	while(ndpagent->thread_running)
	{
		usleep(1000);
		if(!ndpagent->thread_running) break;
		gettimeofday(&curr_time,NULL);
		diff_usec = get_timediff_usec(&curr_time,&last_sent);
		if(diff_usec > NDP_REQ_BCAST_INTERVAL*1000)
		{
			update_ndp_agent(ndpagent);
			//printf("ndp_send_recv() : NDP Request being sent \n");
			ret=send_ndp_req_broadcast(ndpagent);
			if(ret < 0)  {
				fprintf(stderr,"ndp_send_recv(): send_ndp_req_broadcast failed again.. will giveup now a.. and try later .. \n");
				continue;
			} else gettimeofday(&last_sent,NULL);
		 
		}
		
				
		sigemptyset(&blockset);
		sigaddset(&blockset,SIGINT);
		sigaddset(&blockset,SIGTERM);

		FD_ZERO (&reads);
		FD_SET (res_recv_sock, &reads);
		FD_SET (req_recv_sock, &reads);
		totv.tv_sec = timeout_sec;
		totv.tv_usec = timeout_usec;
		
		tots.tv_sec = timeout_sec;
		tots.tv_nsec = timeout_usec*1000;
		
		pthread_sigmask(SIG_BLOCK,&blockset,NULL);
		//status = select (nfds_max, &reads, NULL, NULL, &totv);
		//printf("ndp_send_recv() b4 pselect \n");
		status = pselect (nfds_max, &reads, NULL, NULL, &tots,&emptyset);
      		/* timeout? */
		// printf("ndp_send_recv() after pselect \n");
      		if (status == 0)
       		{
			//printf("ndp_send_recv() : recv timeout (%d s) expired \n", timeout_sec);
			if(!ndpagent->thread_running) break;
			continue;
		}
     		if (status < 0)
       		{
			perror("ndp_send_recv() : recv failed on select");
			if(!ndpagent->thread_running) break;
			continue;
		}
		
		resmsg_avail = (FD_ISSET(res_recv_sock, &reads))?1:0;
		reqmsg_avail = (FD_ISSET(req_recv_sock, &reads))?1:0;
		//printf("ndp_send_recv()  resmsg_avail %d  reqmsg_avail %d \n",resmsg_avail,reqmsg_avail);	
		for(i=0; i<(resmsg_avail + reqmsg_avail); i++)
		{
		 if(resmsg_avail) {
			recv_sock = res_recv_sock;
			resmsg_avail = 0;
		} else if(reqmsg_avail) {
			recv_sock = req_recv_sock;
			reqmsg_avail = 0;
		}
       		if((numbytes = recvfrom(recv_sock, recvbuf, 1500 ,0, (struct sockaddr *)&their_addr,&addr_len)) ==-1) {
			//fprintf(stderr,"ndp_send_recv() recvfrom\n");
                        perror("ndp_send_recv() recvfrom");
   			continue;
                }
  
               recvbuf[numbytes]='\0';
                u_int8_t type = (u_int8_t)recvbuf[0];
                switch (type)
                {
                        case NDP_REQUEST:
				ndp_req = (struct ndp_request  *)recvbuf;
				//printf("ndp_send_recv(): recv %s\n", get_ndp_req_str(ndpagent,ndp_req));
				process_ndp_req(ndpagent,ndp_req);
				break;

                        case NDP_RESPONSE:
				ndp_res = (struct ndp_resp  *)recvbuf;
				//printf("ndp_send_recv(): recv %s\n", get_ndp_res_str(ndpagent,ndp_res));
				process_ndp_res(ndpagent,ndp_res);
			break;
		

			default:	
				fprintf(stderr,"ndp_send_recv() : Wrong Packet with type %d frmc %s\n",type,inet_ntoa(their_addr.sin_addr));	
				break;
		}  // switch
		} // for loop
	} // while loop 
	
	close_ndp_socks(ndpagent);
	printf("ndp_send_recv() thread exiting ..\n");
	ndpagent->thread_running = 0 ; // Flag set to Thread stop
	ndpagent->thread_status = 0 ; // Thread inactive
	return NULL;
}


/*
void *start_wifi_scan(void *arg)
{
	struct ndp_agent *ndpAgent = (struct ndp_agent *)arg;
	char *scancmd="SCAN";
	char scanreply[1024];
	int scanreply_len;
	int wificmd_ret=0;
	printf("start_wifi_scan() start\n");
	//int wifi_command(const char *command, char *reply, size_t *reply_len)
	wifi_start_supplicant();
	while(ndpAgent->thread_running)
	{
		wificmd_ret = wifi_command(scancmd,scanreply,&scanreply_len);
		if(wificmd_ret >=0)
		{
			printf("scanreply : %s ", scanreply);
		} else fprintf(stderr,"start_wifi_scan(): error in wifi_command  ret_val = %d\n",wificmd_ret);
		sleep(2);
	}
	close_ndp_socks(ndpAgent);
	printf("start_wifi_scan() thread exiting ..\n");
	ndpAgent->thread_running = 0 ; // Flag set to Thread stop
	ndpAgent->thread_status = 0 ; // Thread inactive
	return NULL;
}

*/



