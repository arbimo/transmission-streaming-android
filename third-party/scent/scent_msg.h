
#include <stdint.h>
#include "netutils_common.h"
#define MAC_ADDR_LEN 6
//typedef enum  SCENT_Msg_Type { SCENT_CONNECTION_MSG = 1, SCENT_TRUST_MSG =2, SCENT_MEASUREMENT_MSG=3, SCENT_COOPERATE_MSG=4 } ;
 enum   { SCENT_CONNECTION_MSG = 1, SCENT_TRUST_MSG =2, SCENT_MEASUREMENT_MSG=3, SCENT_COOPERATE_MSG=4 } ;

struct Scent_Msg {
	uint8_t msg_type;
	uint16_t msglen;
	uint8_t uid;
	uint8_t mac_addr[MAC_ADDR_LEN];
	uint32_t timestamp;
	uint16_t data_len;
	uint8_t *data;
} __packed;

// Created memory needs to be freed by free
struct Scent_Msg *create_scent_msg(uint8_t type, uint8_t uid, uint8_t *macaddr, uint32_t ts, uint16_t dlen, uint8_t *data);

// Created memory needs to be freed by free
uint8_t* scent_msg_to_str(struct Scent_Msg * scentMsg);

// Created memory needs to be freed by free
struct Scent_Msg *scent_msg_from_str(uint8_t*  msgstr);

void Print_ScentMsg(struct Scent_Msg *scentMsg);