#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "ndp_agent.h"
#include<stdint.h>
// test code 

struct ndp_agent *ndpAgentGlobal;

void  test_update_callback(void *arg,uint8_t rssi,const uint8_t *macaddr,uint8_t uid,uint32_t ts)
{
	
}

static void  ndp_sig_handler(int signo)
{
	if(signo == SIGBUS)
	{
		printf("ndp_sig_handler() Signal SIGBUS received.. just ignoring \n");
		return;
	}
	if ((signo == SIGINT) || (signo == SIGTERM))
	{
		printf("ndp_sig_handler() Signal %s received, exiting now \n", (signo==SIGINT)?"SIGINT":"SIGTERM");
		if(!is_ndp_stopped(ndpAgentGlobal)) stop_ndp_agent(ndpAgentGlobal);
	}

}

int main(int argc, char *argv[]) 
{
	struct ndp_agent ndpAgent;
	uint8_t uid = 2;
	const char *ifname = "eth0";
	char *update_callback_arg = "test callback arg";
	ndpAgentGlobal = &ndpAgent;
	signal(SIGINT,ndp_sig_handler);
	signal(SIGBUS,ndp_sig_handler);
	signal(SIGTERM,ndp_sig_handler);
	
	init_ndp_agent(ndpAgentGlobal,uid, ifname, test_update_callback,update_callback_arg);
	
	if(!is_ndp_stopped(&ndpAgent)) wait_for_ndpthread_exit(&ndpAgent);

	printf("exiting main ()\n");
	return 1;
}
   