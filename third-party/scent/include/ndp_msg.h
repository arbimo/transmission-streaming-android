/** Neighbor Discovery Protocol -NDP*/
#ifndef  NDP_MSG_H__
#define  NDP_MSG_H__

#define NDP_PORT 12000
#define NDP_REQUEST 129  
#define NDP_RESPONSE 131
#define NDP_UNIQUEID 247
/*  NDP_REQ_BCAST_INTERVAL in ms */
#define NDP_REQ_BCAST_INTERVAL 4000
/* NDP_RECV_TIMEOUT in ms */
#define NDP_RECV_TIMEOUT 10
#define MAC_ADDR_LEN 6
#define      IEEE80211_ADDR_LEN 6


#define        __packed        __attribute__((__packed__))

struct ndp_request {
	u_int8_t type;
        u_int8_t uid;
        int seq_count;
	unsigned long ip_addr;
} __packed;

struct ndp_resp {
	u_int8_t type;
        u_int8_t uid;
	u_int8_t freq;
	u_int8_t rssi;
	u_int8_t mac_addr[MAC_ADDR_LEN];
        u_int8_t essid[IEEE80211_ADDR_LEN];
        int seq_count; 
	unsigned long ip_addr;
	struct timeval last_sense;
	//int status;
} __packed;




#endif  // NDP_MSG_H__