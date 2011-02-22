

#ifndef TRUST_COMMON_H
#define TRUST_COMMON_H

#include <stdint.h>
#include "netutils_common.h"
#define  MAX_TRUST_WINDOWS 10

#define  NEIGHBOR_MAX    10
#define MAC_ADDR_LEN 6 

/** Trust value of a peer/node  */
struct PeerTrust {
	uint8_t node_uid;     /* Node unique id */
	uint8_t mac_addr[MAC_ADDR_LEN];  /* Mac address of the node */
	uint8_t trust_val[MAX_TRUST_WINDOWS];  /* stores the trust values evalvated at the last "MAX_TRUST_WINDOWS"  attempts */
	uint8_t current_win_index;  /* current window index  at which the next trust update to bve stored */
	uint16_t update_cnt;    /* Total number of trust updates for this node so far */
	struct timeval last_tv;  /* last updated time  */
	struct PeerTrust *next;
	struct PeerTrust *prev;	
};


struct NeighborTrustList {
	struct PeerTrust *head;
	struct PeerTrust *tail;
	int size;
	//struct PeerTrust  peerTrust[NEIGHBOR_MAX];
};


int GetTrustAverage(const struct PeerTrust  *ptrust);


void InitNeighborTrustList(struct NeighborTrustList *ntrustlist);


int UpdatePeerTrust(struct NeighborTrustList *ntrustlist, int node_id, uint8_t *macaddr, int trustval,struct timeval *update_time);

int UpdatePeerTrustById(struct NeighborTrustList *ntrustlist, uint8_t node_id, int trustval,struct timeval *update_time);

int UpdatePeerTrustByMacAddr(struct NeighborTrustList *ntrustlist, uint8_t *macaddr, int trustval,struct timeval *update_time);


int GetPeerTrustByMacAddr(const struct NeighborTrustList *ntrustlist, uint8_t *macaddr);

int GetPeerTrustById(const struct NeighborTrustList *ntrustlist, uint8_t node_id);


int GetPeerCount(struct NeighborTrustList *ntrustlist);

struct PeerTrust  *GetHead(struct NeighborTrustList *ntrustlist);

int RemoveExpiredPeers(struct NeighborTrustList *ntrustlist, uint32_t expire);

#endif