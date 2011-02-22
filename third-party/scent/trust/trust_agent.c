
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
#include <netdb.h>

#include "netutils_common.h"
#include "scent_msg.h"

#include "trust_common.h"
#include "peer_trustmsg.h"
#include "trust_agent.h"

/** Neighbor Discovery Protocol : NDP  */
#define TAEXIT(x) { fprintf(stderr,"exiting with %d",x); exit(x); }

void *trust_update_to_server(void *arg);

pthread_mutex_t trustAgentNeighborTrustListMutex  = PTHREAD_MUTEX_INITIALIZER;


int get_trustAgent_send_sock() {
	int     reuse=1;
        int int_op = 48,s;
	s =socket(AF_INET, SOCK_DGRAM, 0);
        if (setsockopt(s, IPPROTO_IP, IP_TOS, &int_op, sizeof(int_op)) == -1)
        {
                perror("get_trustAgent_send_sock() : setsockopt-IP_TOS: ");
                TAEXIT(0);
        }

        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse)) == -1)
        {
                perror("get_trustAgent_send_sock() : setsockopt-SO_REUSEADDR: ");
                TAEXIT(0);
        }
	return s;
}



int trustAgent_thread_start(struct trust_agent *trustAgent)
{
	if(!trustAgent) return -1;
	trustAgent->thread_running = 1;
	printf("trustAgent_thread_start() : creating trust_thread \n");
	int res = pthread_create(&(trustAgent->thread_id),NULL,trustAgent->thread_runner, (void *)trustAgent);
	if(res < 0)  {
		perror ("trustAgent_thread_start(): Thread create error \n");
		fprintf(stderr, "trustAgent_thread_start(): Thread creation failed for trust_thread\n");
		trustAgent->thread_running = 0;
		trustAgent->thread_status = 0;
		return -1;
	} else trustAgent->thread_status = 1;
	//pthread_join(trustAgent->thread_id,NULL);
	printf("trustAgent_thread_start():  trust_thread created successfully \n");
	//pthread_join(trustAgent->thread_id,NULL);
	return 1;
}


void stop_trust_agent(struct trust_agent *trustAgent)
{
	if(trustAgent->thread_status) printf("stop_trust_agent(): Stopping trust_thread \n");
	trustAgent->thread_running = 0;
	usleep(1000);
	//pthread_detach(mct->id);
}

void wait_for_trustAgentThread_exit(struct trust_agent *trustAgent)
{
	// extra caoution because  pthread_join does not work well on android

	printf("wait_for_trustthread_exit() : about to check the trust thread \n"); 
	do {
		usleep(1000000);
	} while (trustAgent->thread_status) ;
	
	printf("wait_for_trustthread_exit() : thread_status = STOPPED, Hence  exiting  wait loop now \n"); 
}

void init_trust_agent(struct trust_agent *trustAgent, uint8_t uid, const char *ifname, const char *server_ip,int server_port, int timeout)
{
 	trustAgent->uid = uid;
	struct in_addr servAddr;
	struct hostent *host = gethostbyname(server_ip);
	if (host == NULL) {
		perror("init_trust_agent(): hostname is not found\n");
		exit(0);
	}
	memcpy(&servAddr,host->h_addr, sizeof(servAddr));
	trustAgent->server_ip = (unsigned long)host->h_addr;
	
	//if(inet_aton(server_ip,&servAddr) == 0) TAEXIT(-1);
	trustAgent->server_ip = servAddr.s_addr;
	
	trustAgent->server_port = server_port;

	u_int8_t macaddr[MAC_ADDR_LEN];
	get_if_macaddr(macaddr,ifname);
	memcpy(trustAgent->mac_addr,macaddr, MAC_ADDR_LEN);
	gettimeofday(&(trustAgent->ref_time),NULL);

	uint32_t ipaddr = get_if_ipaddr(ifname);
	trustAgent->ip_addr = ipaddr;
	trustAgent->send_sock = get_trustAgent_send_sock();
	trustAgent->thread_runner = trust_update_to_server;
	printf("init_trust_agent() : trust_agent inilized, ether : %s \n",macaddr_ntos(trustAgent->mac_addr));

	trustAgent->timeout = timeout;
	//trustAgentNeighborTrustListMutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&trustAgentNeighborTrustListMutex);
	InitNeighborTrustList(&trustAgent->neighTrustList);
	pthread_mutex_unlock(&trustAgentNeighborTrustListMutex);

	trustAgent_thread_start(trustAgent);
	
}


void close_trust_socks(struct trust_agent *trustAgent)
{
	if(trustAgent->send_sock) close(trustAgent->send_sock);
	
}


inline int is_trustAgent_stopped(struct trust_agent *trustAgent)
{
	return !(trustAgent->thread_status);
}


void CopyFromPeerTrust(struct  PeerTrustMsg *ptMsg, const struct PeerTrust  *pt)
{
	ptMsg->node_id = pt->node_uid;
	memcpy(ptMsg->mac_addr, pt->mac_addr,MAC_ADDR_LEN);
	ptMsg->trust_val = GetTrustAverage(pt);
	ptMsg->timestamp = pt->last_tv.tv_sec;  // take the sec component only for now.
}

int send_trust_update(struct trust_agent *trustAgent) {
	
	struct sockaddr_in dst_addr;
	struct Scent_Msg *scentMsg;
	struct PeerTrustMsgCombined *comTrustMsg;
	struct timeval time_now;
	uint8_t *sendMsg;
	int ret_value=1;
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_addr.s_addr = trustAgent->server_ip;
	dst_addr.sin_port = htons(trustAgent->server_port);

	pthread_mutex_lock(&trustAgentNeighborTrustListMutex);

	if(trustAgent->neighTrustList.size == 0)
	{
		printf("send_trust_update(): no PeerTrust to send \n");
		pthread_mutex_unlock(&trustAgentNeighborTrustListMutex);
		return 0;
	}
	
	int msgCount = GetPeerCount(&trustAgent->neighTrustList);
	struct PeerTrust *ptrust =  GetHead(&trustAgent->neighTrustList);

	comTrustMsg = CreatePeerTrustMsgCombined(msgCount);
	struct  PeerTrustMsg *ptrustMsg = comTrustMsg->pTrustMsgList;

	int counter  = 0;
	if(msgCount == 0 )  { 
		printf("send_trust_update(): no PeerTrust to send \n");
		pthread_mutex_unlock(&trustAgentNeighborTrustListMutex); 
		return 0;
		 }
	while((ptrust) && (counter < msgCount))
	{
		CopyFromPeerTrust(ptrustMsg,ptrust);
		if(trustAgent->neighTrustList.tail == ptrust) break;
		ptrust = ptrust->next;
		counter++;
		if(counter == msgCount) break;
		ptrustMsg = &(comTrustMsg->pTrustMsgList[counter]);
		
	}

	pthread_mutex_unlock(&trustAgentNeighborTrustListMutex);

	gettimeofday(&time_now,NULL);
	uint32_t ts= get_timediff_sec(&time_now,&trustAgent->ref_time);
	uint8_t *comTrustMsgStr = PeerTrustMsgCombinedToStr(comTrustMsg);
	scentMsg = create_scent_msg(SCENT_TRUST_MSG,trustAgent->uid,trustAgent->mac_addr,ts,comTrustMsg->msg_len,comTrustMsgStr);
	sendMsg = scent_msg_to_str(scentMsg);
	
	ret_value = sendto(trustAgent->send_sock, sendMsg,scentMsg->msglen, MSG_NOSIGNAL, (struct sockaddr *) &dst_addr, sizeof (dst_addr));
	if(ret_value > 0)  {
	 //printf("send_trust_res() : sent %d bytes while scentMsg->msglen = %d \n", ret_value,scentMsg->msglen ); 
		struct Scent_Msg *smg = scent_msg_from_str(sendMsg);
		//Print_ScentMsg(smg);
		free(smg);
	}
	else perror("send_trust_update() : ");

	
	free(comTrustMsgStr);
	FreePeerTrustMsgCombined(comTrustMsg);
	free(scentMsg);
	free(sendMsg);

	return ret_value;
}

void update_trust_agent(struct trust_agent *trustAgent)
{
	// TBD
	uint32_t expire = 1000*trustAgent->timeout;
	RemoveExpiredPeers(&trustAgent->neighTrustList,expire);
}


void *trust_update_to_server(void *arg)
{
	struct trust_agent *trustagent = (struct trust_agent *)arg;
	trustagent->thread_running = 1; // Thread active
	struct timeval curr_time,last_sent;
	int ret = 0; 
	uint32_t diff_msec;
		
	gettimeofday(&curr_time,NULL);
	update_trust_agent(trustagent);
	ret=send_trust_update(trustagent);
	if(ret < 0 )
	{
		fprintf(stderr,"trust_update_to_server(): send_trust_update failed.. will try once more \n");
		ret=send_trust_update(trustagent);
		if(ret < 0) fprintf(stderr,"trust_update_to_server(): send_trust_update filed again.. will giveup now a.. and try later .. \n");
		
	}

	gettimeofday(&last_sent,NULL);
	while(trustagent->thread_running)
	{
		usleep(50);
		if(!trustagent->thread_running) break;
		gettimeofday(&curr_time,NULL);
		diff_msec = get_timediff_msec(&curr_time,&last_sent);
		if(diff_msec >SCENT_TRUST_UPDATE_INTERVAL)
		{
			update_trust_agent(trustagent);
			//printf("trust_update_to_server() : trust_update being sent \n");
			ret=send_trust_update(trustagent);
			if(ret < 0)  {
				fprintf(stderr,"trust_update_to_server(): send_trust_update filed again.. will giveup now a.. and try later .. \n");
				continue;
			} else gettimeofday(&last_sent,NULL);
		 
		}
	update_trust_agent(trustagent);
		

	} // while loop 
	
	close_trust_socks(trustagent);
	printf("trust_update_to_server() thread exiting ..\n");
	trustagent->thread_running = 0 ; // Flag set to Thread stop
	trustagent->thread_status = 0 ; // Thread inactive
	return NULL;
}




int update_peerTrust(struct trust_agent *trustAgent, int node_id, uint8_t *macaddr, int trustval,struct timeval *update_time)
{
	int retval = 0 ;
	pthread_mutex_lock(&trustAgentNeighborTrustListMutex);
	retval = UpdatePeerTrust(&trustAgent->neighTrustList,node_id,macaddr,trustval,update_time);
	pthread_mutex_unlock(&trustAgentNeighborTrustListMutex);
	return retval;
}

void signal_urgent_update(struct trust_agent * trustAgent)
{
	send_trust_update(trustAgent);
}
