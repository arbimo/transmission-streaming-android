/** Neighbor Discovery Protocol -NDP*/
#ifndef  NDP_AGENT_H__
#define  NDP_AGENT_H__
#define MAC_ADDR_LEN 6
#define      IEEE80211_ADDR_LEN 6

#include <pthread.h>
#include<stdint.h>
#include <string.h>
#define  WIFIMONPORT = 12011;

struct ndp_agent
{
	u_int8_t uid;
	uint8_t ifname[20];
	int	req_count;
	int	req_recv_sock;
	int	res_recv_sock;
	int	req_send_sock;
	int	res_send_sock;
	unsigned long ip_addr;
	unsigned long bcast_addr;
	u_int8_t mac_addr[MAC_ADDR_LEN];
        u_int8_t essid[IEEE80211_ADDR_LEN];
	u_int8_t freq;
	u_int8_t curr_rssi;
	struct timeval ref_time;
	struct movi_client *mc;
	char ndp_str[150];
	void (*update_neighbor_callback)(void *callback_arg, uint8_t rssi, const uint8_t *mac_addr, uint8_t uid, uint32_t timestamp);
	void *update_arg;
	pthread_t  thread_id;
	int	thread_running;
	int	thread_status;
	void *(*thread_runner)(void *);
	
};

void init_ndp_agent(struct ndp_agent *ndpAgent,uint8_t uid, const char *ifname, void  update_callback(void *,uint8_t,const uint8_t *,uint8_t,uint32_t), void *update_callback_arg);
void stop_ndp_agent(struct ndp_agent *ndpAgent);
inline int is_ndp_stopped(struct ndp_agent *ndpAgent);
void wait_for_ndpthread_exit(struct ndp_agent *ndpAgent);

#endif  // NDP_AGENT_H__
