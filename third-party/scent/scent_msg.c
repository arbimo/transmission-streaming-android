#include <stdlib.h>
#include <string.h>
//#include <error.h>
#include <stdio.h>
#include "scent_msg.h"


// Created memory needs to be freed by free
struct Scent_Msg *create_scent_msg(uint8_t type, uint8_t uid, uint8_t *macaddr, uint32_t ts, uint16_t dlen, uint8_t *data)
{
	struct Scent_Msg *scentMsg = (struct Scent_Msg *)malloc(sizeof(struct Scent_Msg));
	scentMsg->msg_type = type;
	scentMsg->uid = uid;
	memcpy(scentMsg->mac_addr,macaddr,MAC_ADDR_LEN);
	scentMsg->timestamp = ts;
	scentMsg->data_len = dlen;
	scentMsg->data = data;
	scentMsg->msglen = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t) + MAC_ADDR_LEN*sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + dlen;
	return scentMsg;
}

// Created memory needs to be freed by free
uint8_t*  scent_msg_to_str(struct Scent_Msg * scentMsg)
{
	int headerLen = scentMsg->msglen - scentMsg->data_len;
	uint8_t *msgStr = (uint8_t *)malloc(scentMsg->msglen);
	memcpy(msgStr,(uint8_t *)scentMsg,headerLen);
	memcpy(msgStr+headerLen,scentMsg->data,scentMsg->data_len);
	return msgStr;
}


// Created memory needs to be freed by free
struct Scent_Msg *scent_msg_from_str(uint8_t*  msgstr)
{
	struct Scent_Msg *scentMsg = (struct Scent_Msg *)malloc(sizeof(struct Scent_Msg));
	int headerLen = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t) + MAC_ADDR_LEN*sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t);
	memcpy((uint8_t *)scentMsg,msgstr,headerLen);
	if(scentMsg->data_len + headerLen != scentMsg->msglen)
	{
		fprintf(stderr,"scent_msg_from_str(): Error in length values scentMsg->data_len = %d, headerLen = %d, scentMsg->msglen = %d \n",scentMsg->data_len,headerLen,scentMsg->msglen);
		fprintf(stderr,"scent_msg_from_str():  Length values from the msg-string data_len = %d, msg_len = %d \n",*((uint16_t *)(msgstr+9)), *((uint16_t *)(msgstr+1)) );
	}
	scentMsg->data = msgstr+headerLen;
	//memcpy(scentMsg.data,msgstr+headerLen,scentMsg->data_len);
	return scentMsg;
}

void Print_ScentMsg(struct Scent_Msg *scentMsg)
{
	printf("Scent_Msg: type = %d, msglen = %d , uid = %d, mac_addr = %s, timestamp = %d , data_len = %d, smsg->data = %s \n", \
	scentMsg->msg_type,scentMsg->msglen,scentMsg->uid,macaddr_ntos(scentMsg->mac_addr),scentMsg->timestamp,scentMsg->data_len,(scentMsg->data)?"valid":"in valid");
}

