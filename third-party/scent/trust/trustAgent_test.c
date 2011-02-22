#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "trust_agent.h"
#include <stdint.h>
#include "trust_update.h"
#include "scent_player_status.h"

// test code 

struct trust_agent *trustAgentGlobal;

static void  trustAgent_sig_handler(int signo)
{
	if(signo == SIGBUS)
	{
		printf("trustAgent_sig_handler() Signal SIGBUS received.. just ignoring \n");
		return;
	}
	if ((signo == SIGINT) || (signo == SIGTERM))
	{
		printf("trustAgent_sig_handler() Signal %s received, exiting now \n", (signo==SIGINT)?"SIGINT":"SIGTERM");
		if(!is_trustAgent_stopped(trustAgentGlobal)) stop_trust_agent(trustAgentGlobal);
	}

}

int scent_video_player_running;

int main(int argc, char *argv[]) 
{
	struct trust_agent trustAgent;
	uint8_t uid = 32;
	const char *ifname = "eth0";
	trustAgentGlobal = &trustAgent;
        signal(SIGINT,trustAgent_sig_handler);
        signal(SIGBUS,trustAgent_sig_handler);
	signal(SIGTERM,trustAgent_sig_handler);
	//init_trust_agent(struct trust_agent *trustAgent,uint8_t uid, const char *ifname, const char *server_ip,int server_port)	
	init_trust_agent(trustAgentGlobal,uid, ifname, "10.10.3.1", 12000,1000*PTRUST_TIMEOUT);
	uint8_t mymac_addr[MAC_ADDR_LEN];
	memcpy(mymac_addr,trustAgentGlobal->mac_addr,MAC_ADDR_LEN);
	uint8_t node_mac_addr[MAC_ADDR_LEN];
	int i = 0, trustval = 0; struct timeval now;
	struct peer_update_agent pua;
	init_peer_update_agent(&pua,trustAgentGlobal,uid,mymac_addr,trustAgent_sig_handler);
	scent_video_player_running = 1;
	printf("Updating the PeerTrust \n");
	for(i=0; i<100; i++)
	{	gettimeofday(&now,NULL);
		trustval = 1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0)));
		node_mac_addr[MAC_ADDR_LEN-1] = mymac_addr[MAC_ADDR_LEN-1] + (i%9) ;
		update_peerTrust(trustAgentGlobal,i%9,node_mac_addr,trustval,&now);
		add_peer_to_list(&pua,i%9,node_mac_addr,trustval);
		usleep(1000);
	}
		
	

	if(!is_trustAgent_stopped(&trustAgent)) wait_for_trustAgentThread_exit(&trustAgent);
	stop_peer_update_agent(&pua);
	printf("exiting main ()\n");
	return 1;
}
   
