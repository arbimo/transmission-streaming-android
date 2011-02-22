/** Trust Agent */
#ifndef  TRUSTAGENT_AGENT_H__
#define  TRUSTAGENT_AGENT_H__
#define MAC_ADDR_LEN 6
#define IEEE80211_ADDR_LEN 6

#include<stdint.h>
#include <string.h>
#include <pthread.h>

#include "trust_common.h"
// in milli seconds
#define SCENT_TRUST_UPDATE_INTERVAL  100
/* an average value of trust of range 0 -9 */
#define  DEFAULT_TRUST    5

#define PTRUST_TIMEOUT  600

struct trust_agent {
	uint8_t uid;
	int	send_sock;
	unsigned long ip_addr;
	uint8_t mac_addr[MAC_ADDR_LEN];
	unsigned long server_ip;
	int	server_port;
	struct NeighborTrustList neighTrustList;
	struct timeval ref_time;
	char trust_str[150];
	int  timeout; // in milli seconds , a node will be removed if no updates after timeout milliseconds
	pthread_t  thread_id;
	int	thread_running;
	int	thread_status;
	void *(*thread_runner)(void *);
};

void init_trust_agent(struct trust_agent *trustAgent,uint8_t uid, const char *ifname, const char *server_ip,int server_port, int timeout);

void stop_trust_agent(struct trust_agent *trustAgent);
inline int is_trustAgent_stopped(struct trust_agent *trustAgent);
void wait_for_trustAgentThread_exit(struct trust_agent *trustAgent);
int update_peerTrust(struct trust_agent *trustAgent, int node_id, uint8_t *macaddr, int trustval,struct timeval *update_time);
void signal_urgent_update(struct trust_agent * trustAgent);
#endif  // TRUSTAGENT_AGENT_H__

