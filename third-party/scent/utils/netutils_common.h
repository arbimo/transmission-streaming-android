
#ifndef NETUTILS_COMMON_H__
#define NETUTILS_COMMON_H__ 1
#include <sys/time.h>
#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <stdint.h>

#define MAC_ADDR_LEN 6
#ifndef IEEE80211_ADDR_LEN
#define IEEE80211_ADDR_LEN 9
#endif

#ifndef __packed
#define  __packed  __attribute__((__packed__))
#endif

#ifndef DEFAULT_WIFI_RSSI_MIN
#define DEFAULT_WIFI_RSSI_MIN 40
#endif
#ifndef DEFAULT_WIFI_RSSI_MAX
#define DEFAULT_WIFI_RSSI_MAX 100
#endif


void get_if_macaddr(uint8_t *mac, const char *interface_name);

uint32_t  get_if_ipaddr(const char *interface_name);

uint32_t  get_if_bcastaddr(const char *interface_name);

char *macaddr_ntos(const uint8_t *eth_addr);

uint32_t get_timediff_usec(struct timeval* now, struct timeval* old);

uint32_t get_timediff_msec(struct timeval* now, struct timeval* old);

uint32_t get_timediff_sec(struct timeval* now, struct timeval* old);

void get_local_essid(uint8_t *essidp) ;

uint8_t get_local_wifi_freq(const char *interface_name);

uint8_t get_local_wifi_rssi(const char *interface_name);

#endif 