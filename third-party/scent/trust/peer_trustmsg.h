

#ifndef PEER_TRUST_MSG_H
#define PEER_TRUST_MSG_H

#include <stdint.h>
#include "netutils_common.h"

#define MAC_ADDR_LEN 6 

/** Trust Messages for a neighbor peer */

struct  PeerTrustMsg {
	uint8_t node_id;
	uint8_t mac_addr[MAC_ADDR_LEN];
	uint8_t trust_val;
	uint32_t timestamp;
} __packed;

/** Combined list of neighbor PeerTrust messages top be sent */

struct PeerTrustMsgCombined {
	uint16_t msg_len;
	uint8_t  msg_count;
	struct  PeerTrustMsg *pTrustMsgList;
} __packed;
 
// New memory for the obj and the pointer inside created and need to be freed by FreePeerTrustMsgCombined()
struct PeerTrustMsgCombined *CreatePeerTrustMsgCombined(uint8_t  msg_count);

void FreePeerTrustMsgCombined(struct PeerTrustMsgCombined *ptrmsgcm);


//Only New memory for the obj created (not for the pointer inside) and only need to be freed by free()
uint8_t *PeerTrustMsgCombinedToStr(struct PeerTrustMsgCombined *ptmc);

// New memory created and need to be freed by free()
struct PeerTrustMsgCombined *CreatePeerTrustMsgCombinedFromStr(uint8_t *fromStr);

#endif