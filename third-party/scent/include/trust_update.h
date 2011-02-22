#ifndef TRUST_UPDATE_H__
#define TRUST_UPDATE_H__ 1
#include "trust_agent.h"

#define MAX_PEERS 10
#define PEER_UPDATE_PORT  8890
#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN 6
#endif

#define TU_SERVER_PORT 13000
/*  1000 MICRO SECONDS */
#define TU_UPDATE_INTERVAL 100

#define PEER_TRUST_UPDATE_REQUEST  15
#define PEER_TRUST_UPDATE_RESPONSE 225

#ifndef DEFAULT_TRUST_VALUE
#define DEFAULT_TRUST_VALUE 4
#endif


struct peer_entity
{
	uint8_t uid;
	uint8_t mac_addr[MAC_ADDR_LEN];
};

struct peer_update
{
	uint8_t uid;
	uint8_t mac_addr[MAC_ADDR_LEN];
	uint8_t trust_val;
  
} __packed;

struct peer_update_msg
{
	uint8_t type;
	uint8_t uid;
	uint8_t mac_addr[MAC_ADDR_LEN];
	uint8_t neighborCount;
	struct peer_update nlist[MAX_PEERS];	
} __packed;


struct peer_update_agent
{
	struct trust_agent  *trustAgent;
	uint8_t uid;
	uint8_t mac_addr[MAC_ADDR_LEN];
	int neighbor_count;
	struct peer_update nlist[MAX_PEERS];
	int tu_server_running;
	int tu_client_running;
	int tu_server_sock;
	int tu_client_sock;
	pthread_t thread_id;
	int thread_status;
	void (*exit_func)(int signo);
	
};

void init_peer_update_agent(struct peer_update_agent *pua,struct trust_agent  *tra, uint8_t nodeid,uint8_t *macaddr,void exitfunc(int));
int add_peer_to_list(struct peer_update_agent *pua, int uid, uint8_t *mac_addr,int tval);
int remove_peer_from_list(struct peer_update_agent *pua, uint8_t *mac_addr);
int send_update_req(struct peer_update_agent *pua);
int recv_update_res(struct peer_update_agent *pua);
int update_to_trust_agent(struct peer_update_agent *pua);
void stop_peer_update_agent(struct peer_update_agent *pua);
void print_pumsg_values (struct peer_update_msg *pumsg);
void stop_pua_client_conn(struct peer_update_agent *pua);
#endif //TRUST_UPDATE_H__
