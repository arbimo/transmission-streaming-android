
#include <sys/ipc.h>
#include <sys/stat.h>
//#include <sys/file.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
//#include <err.h>

#include "netutils_common.h"

void get_if_macaddr(uint8_t * mac, const char *interface_name)
{
        struct ifreq ifr_;
        struct sockaddr *sa;
        uint8_t *addr;
        strncpy(ifr_.ifr_name, interface_name, sizeof (ifr_.ifr_name));
	int ctrl_socket =socket(AF_INET, SOCK_DGRAM, 0);
        if (ioctl (ctrl_socket, SIOCGIFHWADDR, &ifr_) < 0)
	{
         	perror("get_if_macaddr() ioctl on ctrl_socket failed");
	 	fprintf(stderr,"get_if_macaddr() ioctl on ctrl_socket failed  for %s\n", ifr_.ifr_name);
	}
        sa = &(ifr_.ifr_hwaddr);
        addr = (uint8_t * ) sa->sa_data;
        memcpy(mac, sa->sa_data, 6);
	close(ctrl_socket);
        return;
}


uint32_t  get_if_ipaddr(const char *interface_name)
{
	struct ifreq ifr_;
        struct sockaddr_in *sin;
        sin = (struct sockaddr_in *) &ifr_.ifr_addr;
 	strncpy(ifr_.ifr_name, interface_name, sizeof (ifr_.ifr_name));
	int ctrl_socket =socket(AF_INET, SOCK_DGRAM, 0);
        if (ioctl (ctrl_socket, SIOCGIFADDR, &ifr_) < 0)
	{
                perror("get_if_ipaddr() ioctl on ctrl_socket failed");
		fprintf(stderr,"get_if_ipaddr()ioctl on ctrl_socket failed  for %s\n", ifr_.ifr_name);
	}
        printf("get_ifip_addr() : ip %u - %s\n",  (uint32_t)(sin->sin_addr.s_addr), inet_ntoa(sin->sin_addr));
	close(ctrl_socket);
	return sin->sin_addr.s_addr; 
}

uint32_t  get_if_bcastaddr(const char *interface_name)
{
	struct ifreq ifr_;
        struct sockaddr_in *sin;
        sin = (struct sockaddr_in *) &ifr_.ifr_addr;
 	strncpy(ifr_.ifr_name, interface_name, sizeof (ifr_.ifr_name));
	int ctrl_socket =socket(AF_INET, SOCK_DGRAM, 0);
        if (ioctl (ctrl_socket, SIOCGIFBRDADDR, &ifr_) < 0)
	{
                perror("get_if_bcastaddr() ioctl on ctrl_socket failed");
		fprintf(stderr,"get_if_bcastaddr() ioctl on ctrl_socket failed  for %s\n", ifr_.ifr_name);
	}
        printf("get_ifbcast_addr () ip %u - %s\n",  (uint32_t)(sin->sin_addr.s_addr), inet_ntoa(sin->sin_addr));
	close(ctrl_socket);
	return sin->sin_addr.s_addr; 
}

char  ether_addr_str_g[MAC_ADDR_LEN*3+1];
char *macaddr_ntos(const uint8_t *eth_addr)
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

uint32_t get_timediff_usec(struct timeval* now, struct timeval* old)
{
		uint32_t sec_diff = now->tv_sec - old->tv_sec;
		if(sec_diff > UINT32_MAX/1000000) return UINT32_MAX;
		uint32_t diff = sec_diff*1000000 + (uint32_t)(now->tv_usec - old->tv_usec);
		return diff;
}

uint32_t get_timediff_msec(struct timeval* now, struct timeval* old)
{
		uint32_t sec_diff = now->tv_sec - old->tv_sec;
		if(sec_diff > UINT32_MAX/1000000) return UINT32_MAX;
		uint32_t diff = 0;
		if(now->tv_usec >= old->tv_usec)
		diff = sec_diff*1000 + (uint32_t)(now->tv_usec - old->tv_usec)/1000;
		else  diff = (sec_diff -1 )*1000 + (uint32_t)( (1000000 + now->tv_usec) - old->tv_usec)/1000;
		return diff;
}

uint32_t get_timediff_sec(struct timeval* now, struct timeval* old)
{
		uint32_t sec_diff = now->tv_sec - old->tv_sec;
		if(sec_diff > UINT32_MAX/1000000) return UINT32_MAX;
		long long int usec_diff = now->tv_usec - old->tv_usec;
		int offset = (usec_diff <= (-500000))?(-1):((usec_diff >= 500000)?1 :0); 
			uint32_t diff = sec_diff + offset ;
		return diff;
}

void get_local_essid(uint8_t *essidp) // to be implemented 
{
	memset(essidp,0,IEEE80211_ADDR_LEN);
	strcpy((char *)essidp, "scentap1");  // just initial value .. need to be implemented ...
}

uint8_t get_local_wifi_freq(const char *interface_name) // to be implemented 
{
	return  7 ;// just initial value .. need to be implemented ..
}

uint8_t get_local_wifi_rssi(const char *interface_name) // to be implemented 
{
	//return  88 ;// just initial value .. need to be implemented ..
	int rsssi_range = DEFAULT_WIFI_RSSI_MAX-DEFAULT_WIFI_RSSI_MIN;
	 //srandom(SEED);
	//int rssi = ((random()*rsssi_range)/RAND_MAX ) + DEFAULT_WIFI_RSSI_MIN;
	uint8_t rssi =  ( ( random() % rsssi_range ) + DEFAULT_WIFI_RSSI_MIN );
	return  rssi;
}
