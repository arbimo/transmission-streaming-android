
#include "trust_common.h"
#include "netutils_common.h"
#include <string.h>
#include <stdlib.h>

static char  ether_addr_str_g[MAC_ADDR_LEN*3+1];
char *macaddress_ntos(const uint8_t *eth_addr)
{
	int i = 0;
	char *addr_str = &(ether_addr_str_g[0]);
	memset(addr_str,0,(MAC_ADDR_LEN*3+1));
	//printf("macaddr_ntos() mac_addr = ");
	for(i=0; i<MAC_ADDR_LEN; i++)
	{
		if(i == MAC_ADDR_LEN-1)
		{
			sprintf(addr_str,"%02x",eth_addr[i]);
			addr_str += 2;
			*addr_str ='\0';
			//printf("%02x\n",eth_addr[i]);
			break;
		} else  {
			sprintf(addr_str,"%02x:",eth_addr[i]);
			//printf("%02x:",eth_addr[i]);
			addr_str += 3 ;
		}
	}
	addr_str = &(ether_addr_str_g[0]);
	//printf("macaddr_ntos() : ether_addr = %s\n",addr_str);
	return addr_str;
}

int GetTrustAverage(const struct PeerTrust  *ptrust)
{
	if(!ptrust) return -1;
	int itemcount = (ptrust->update_cnt >= MAX_TRUST_WINDOWS) ? MAX_TRUST_WINDOWS : ptrust->update_cnt;
	if(itemcount <= 0) return -1;
	int i = 0; int sum = 0;
	for (i=0; i < itemcount; i++ ) sum += ptrust->trust_val[i];
	return sum/itemcount;
}


void InitNeighborTrustList(struct NeighborTrustList *ntrustlist)
{
	ntrustlist->head = NULL;
	ntrustlist->tail = NULL;
	ntrustlist->size = 0;
}

struct PeerTrust  *FindPeerTrustElementById(const struct NeighborTrustList *ntrustlist, int node_id)
{
	if((ntrustlist->size == 0) ||  (ntrustlist->head == NULL)) return NULL;
	struct PeerTrust  *ptrusth = ntrustlist->head;
	struct PeerTrust  *ptrustt = ntrustlist->tail;
	struct PeerTrust  *ptrust_found = NULL; 
	int i = 0;
	do  {
		if(ptrusth->node_uid == node_id)  {
			ptrust_found = ptrusth;
			break;
		}

		if(ptrustt->node_uid == node_id)  {
			ptrust_found = ptrustt; 
			break;
		}

		ptrusth = ptrusth->next;
		ptrustt = ptrustt->prev;
		i++;
	} while((ptrusth != NULL) && (ptrustt != NULL) && (i <= ntrustlist->size/2));
	if((ptrust_found)  && (i > 1+ntrustlist->size/2))  fprintf(stderr,"FindPeerTrustElement() error: the counter (%d) should not be greater than the (1+listSize/2)(%d) \n", i,(1+ntrustlist->size/2) );
	return ptrust_found;
}

struct PeerTrust  *FindPeerTrustElementByMacAddr(const struct NeighborTrustList *ntrustlist,uint8_t *macaddr)
{
	if((ntrustlist->size == 0) ||  (ntrustlist->head == NULL)) return NULL;
	struct PeerTrust  *ptrusth = ntrustlist->head;
	struct PeerTrust  *ptrustt = ntrustlist->tail;
	struct PeerTrust  *ptrust_found = NULL;
	int i = 0;
	do {
		if(memcmp(ptrusth->mac_addr,macaddr,MAC_ADDR_LEN)==0) {
			ptrust_found = ptrusth;
			break;
		}
		if(memcmp(ptrustt->mac_addr,macaddr,MAC_ADDR_LEN)==0) {
			ptrust_found = ptrustt;
			break;
		}
		
		ptrusth = ptrusth->next;
		ptrustt = ptrustt->prev;
		i++;
	} while((ptrusth != NULL)  && (ptrustt != NULL) && (i <= ntrustlist->size/2));
	if(i > ntrustlist->size/2) fprintf(stderr,"FindPeerTrustElement() error the counter (%d) should be less than the list-size(%d)/2 \n", i,ntrustlist->size );
	return ptrust_found;
}

struct PeerTrust  *FindPeerTrustElement(const struct NeighborTrustList *ntrustlist, int node_id, uint8_t *macaddr)
{
	int key = (node_id<0)?((!macaddr)?0:1):((!macaddr)?2:3);
	switch(key) {
		case 0: fprintf(stderr,"FindPeerTrustElement() : both node-id and macaddr are invalid \n"); 
			return NULL; 
			
		case 1:	return FindPeerTrustElementByMacAddr(ntrustlist,macaddr);
			 
		case 2: return FindPeerTrustElementById(ntrustlist,node_id);

		case 3: break;
	}
	
	struct PeerTrust  *ptrust = FindPeerTrustElementById(ntrustlist,node_id);
	if(ptrust == NULL) return ptrust;
	if(memcmp(ptrust->mac_addr,macaddr,MAC_ADDR_LEN) == 0) return ptrust;
	else {
		fprintf(stderr,"FindPeerTrustElement() PeerTrust Element found by the node_id  has a different mac_address (%s), to the given mac_addr (%s) \n", macaddress_ntos(ptrust->mac_addr),macaddress_ntos(macaddr));
		ptrust = FindPeerTrustElementByMacAddr(ntrustlist,macaddr);
	}
	return  ptrust;
}

int UpdatePeerTrust(struct NeighborTrustList *ntrustlist, int node_id, uint8_t *macaddr, int trustval,struct timeval *update_time)
{
	struct PeerTrust  *ptrust = NULL;
	if(ntrustlist->size > 0)  ptrust = FindPeerTrustElement(ntrustlist,node_id,macaddr);
	if(ptrust == NULL)
	{
		// create new  PeerTrust element
		printf("UpdatePeerTrust(): create new  PeerTrust element for uid = %d, trustval = %d, mac_addr = %s , peersCount = %d\n",node_id,trustval, macaddr_ntos(macaddr), ntrustlist->size );
		ptrust = (struct PeerTrust *) malloc(sizeof(struct PeerTrust));
		ptrust->node_uid = node_id;
		memcpy(ptrust->mac_addr,macaddr,MAC_ADDR_LEN);
		ptrust->trust_val[0] = trustval;
		ptrust->current_win_index = 1;
		ptrust->update_cnt = 1;
		ptrust->last_tv = *update_time;
		ptrust->prev = NULL;
		ptrust->next = NULL;
		
		// add new PeerTrust element to the list
		if(ntrustlist->size == 0)  { ntrustlist->head = ntrustlist->tail = ptrust; }
		else {
			struct PeerTrust  *tmp = ntrustlist->tail;
			if(tmp->next != NULL)  { fprintf(stderr,"UpdatePeerTrust() error : ntrustlist->tail->next is not NULL!!!\n"); return -1; }
			tmp->next = ptrust;
			ptrust->prev = tmp;
			ntrustlist->tail = ptrust;
			}
		ntrustlist->size++;
	} else {
		//peerTrust already exists, just update the trust value and timestamp
		printf("UpdatePeerTrust(): peerTrust already exists, updating   the PeerTrust element for uid = %d, trustval = %d, mac_addr = %s, peersCount = %d \n",node_id,trustval, macaddr_ntos(macaddr), ntrustlist->size);
		ptrust->trust_val[ptrust->current_win_index] = trustval;
		ptrust->current_win_index++;
		ptrust->update_cnt++;
		ptrust->last_tv = *update_time;
		ptrust->current_win_index = (ptrust->current_win_index % MAX_TRUST_WINDOWS);  // cap the current_win_index
	}
	return 1;
}



int UpdatePeerTrustById(struct NeighborTrustList *ntrustlist, uint8_t node_id, int trustval,struct timeval *update_time)
{
	return UpdatePeerTrust(ntrustlist,node_id,NULL,trustval,update_time);
	
}

int UpdatePeerTrustByMacAddr(struct NeighborTrustList *ntrustlist, uint8_t *macaddr, int trustval, struct timeval *update_time)
{
	return UpdatePeerTrust(ntrustlist,-1,macaddr,trustval,update_time);
	
}

int GetPeerTrustByMacAddr(const struct NeighborTrustList *ntrustlist, uint8_t *macaddr)
{
	struct PeerTrust  *ptrust = FindPeerTrustElementByMacAddr(ntrustlist,macaddr);
	if(ptrust == NULL)  {
		fprintf(stderr,"GetPeerTrust(): couldn't find a PeerTrust element with the mac-addr (%s)\n",macaddress_ntos(macaddr));
		return -1;
	}
	return GetTrustAverage(ptrust);
}

int GetPeerTrustById(const struct NeighborTrustList *ntrustlist, uint8_t node_id)
{
	struct PeerTrust  *ptrust = FindPeerTrustElementById(ntrustlist,node_id);
	if(ptrust == NULL)  {
		fprintf(stderr,"GetPeerTrust(): couldn't find a PeerTrust element with the node_id (%d)\n",node_id);
		return -1;
	}
	return GetTrustAverage(ptrust);
}
int GetPeerCount(struct NeighborTrustList *ntrustlist)
{
	return ntrustlist->size;
}

struct PeerTrust  *GetHead(struct NeighborTrustList *ntrustlist)
{
	return ntrustlist->head;
}
void PrintPeersList(struct NeighborTrustList *ntrustlist)
{
	struct PeerTrust  *ptrust = NULL;
	printf("PeerTrustList\n");
	printf("-------------\n");
	printf("Node	Mac-Address\n");
	for(ptrust = ntrustlist->head; ptrust != NULL; ptrust = ptrust->next)
	{
		if(!ptrust) break;
		printf(" %d  \t  %s \n", ptrust->node_uid,macaddress_ntos(ptrust->mac_addr));
	}
	printf("-------------\n\n");
}

int RemoveExpiredPeers(struct NeighborTrustList *ntrustlist, uint32_t expire)
{
	
	//struct PeerTrust  *ptrusth = ntrustlist->head;
	//struct PeerTrust  *ptrustt = ntrustlist->tail;
	struct PeerTrust  *ptrust = ntrustlist->head;
	struct PeerTrust  *tmpptrust = ntrustlist->head;
	if(( ntrustlist->size < 1) || (!ptrust) )
	return 0;
	int i = 0;
	int removed_count = 0;
	struct timeval curr_time;
	gettimeofday(&curr_time,NULL);
	uint32_t ref_usec;
	int rem_node = 0;
	char rem_macaddr[32];
	do {
		if((!ptrust) || (ptrust == ntrustlist->tail) ) break;
		ref_usec = get_timediff_usec(&curr_time,&(ptrust->last_tv));
		if(ref_usec > expire)
		{
			printf("RemoveExpiredPeers() : peer_trust expired.. removing peer (%d : %s) from the list, size before removal = %d \n",ptrust->node_uid,macaddress_ntos(ptrust->mac_addr) , ntrustlist->size);
			rem_node = ptrust->node_uid;
			strcpy(rem_macaddr,macaddress_ntos(ptrust->mac_addr));

			//if(i == 0) // means head
			if(ptrust->prev == NULL) // means head
			{
				if((ptrust->next == NULL) && (ntrustlist->size > 1))
					       fprintf(stderr, "RemoveExpiredPeers() ntrustlist->head->next == NULL, But ntrustlist->size (%d) > 1 \n",ntrustlist->size);
				if(ptrust->next) ptrust->next->prev = NULL;
				ntrustlist->head = ptrust->next;
			} else {
				if(ptrust->prev) ptrust->prev->next = ptrust->next;
				if(ptrust->next) ptrust->next->prev = ptrust->prev;
			}
			tmpptrust = ptrust->next;
			free(ptrust);
			ptrust = tmpptrust;
			ntrustlist->size--;
			removed_count++;
			printf("RemoveExpiredPeers() : peer_trust expired.. peer (%d : %s) removed from the list, now size %d, removed_count %d  \n",rem_node,rem_macaddr, ntrustlist->size,removed_count);
			PrintPeersList(ntrustlist);
		} else  ptrust = ptrust->next;
		i++;
	} while(ptrust );
		if(removed_count) { printf("RemoveExpiredPeers() : removed %d peers \n\n",removed_count); PrintPeersList(ntrustlist); }
	return removed_count;

}
