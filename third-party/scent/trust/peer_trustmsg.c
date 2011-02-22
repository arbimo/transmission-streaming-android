#include "peer_trustmsg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

struct PeerTrustMsgCombined *CreatePeerTrustMsgCombined(uint8_t  msg_count)
{
	struct PeerTrustMsgCombined *com_trmsg =  (struct PeerTrustMsgCombined *)malloc(sizeof(struct PeerTrustMsgCombined));
	if(!com_trmsg)  {
		perror("create_Combined_Trust_Msg() : malloc failed to create Combined_Trust_Msg object : ");
		fprintf(stderr, "create_Combined_Trust_Msg() : malloc failed to create Combined_Trust_Msg object\n");
		return NULL;
	}
	com_trmsg->msg_count = msg_count;
	int headerLen = sizeof(uint16_t) + sizeof(uint8_t);
	com_trmsg->msg_len = headerLen + msg_count*sizeof(struct  PeerTrustMsg);
	//com_trmsg->to_str = (uint8_t *)malloc(com_trmsg->msg_len);
	//memcpy(com_trmsg->to_str,(uint16_t *)com_trmsg, headerLen);
		
	com_trmsg->pTrustMsgList = (struct  PeerTrustMsg*)malloc(msg_count*sizeof(struct  PeerTrustMsg));
	//com_trmsg->pTrustMsgList = (struct  PeerTrustMsg *)(com_trmsg->to_str + headerLen);
	if(!com_trmsg->pTrustMsgList)  {
		perror("create_Combined_Trust_Msg() : malloc failed to create Trust_Msg list : ");
		fprintf(stderr, "create_Combined_Trust_Msg() : malloc failed to create Trust_Msg list \n");
		return NULL;
	}

	return com_trmsg;
}


void FreePeerTrustMsgCombined(struct PeerTrustMsgCombined *com_trmsg)
{
	if(!com_trmsg) return;
	if(com_trmsg->pTrustMsgList) free(com_trmsg->pTrustMsgList);
	else fprintf(stderr, "free_Combined_Trust_Msg: com_trmsg->pTrustMsgList is empty\n");
	free(com_trmsg);
	com_trmsg = NULL;

}

struct PeerTrustMsgCombined *CreatePeerTrustMsgCombinedFromStr(uint8_t *fromStr)
{
	struct PeerTrustMsgCombined *com_trmsg =  (struct PeerTrustMsgCombined *)malloc(sizeof(struct PeerTrustMsgCombined));
	if(!com_trmsg)  {
		perror("CreatePeerTrustMsgCombinedFromStr() : malloc failed to create Combined_Trust_Msg object : ");
		fprintf(stderr, "CreatePeerTrustMsgCombinedFromStr() : malloc failed to create Combined_Trust_Msg object\n");
		return NULL;
	}
	com_trmsg->msg_len = *((uint16_t *)fromStr);
	com_trmsg->msg_count =  *((uint8_t *)(fromStr+sizeof(uint16_t)));
	com_trmsg->pTrustMsgList = (struct  PeerTrustMsg *)(fromStr+sizeof(uint8_t)+sizeof(uint16_t));
	return com_trmsg;
}


// Created memory needs to be freed by free
uint8_t *PeerTrustMsgCombinedToStr(struct PeerTrustMsgCombined *com_trmsg)
{
	uint8_t *to_str = (uint8_t *)malloc(com_trmsg->msg_len);
	int headerLen = sizeof(uint16_t) + sizeof(uint8_t);
	memcpy(to_str,(uint8_t *)com_trmsg, headerLen);
	memcpy(to_str+headerLen,(uint8_t *)com_trmsg->pTrustMsgList,com_trmsg->msg_len-headerLen);
	return to_str;
}


