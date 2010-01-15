/* $Id$
 */
#ifndef _XAPDEF_H
#define _XAPDEF_H

#ifdef WIN32
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

void cfmakeraw(struct termios *termios_p);
#endif

#ifdef LINUX
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#endif

extern const char* XAP_ME;
extern const char* XAP_SOURCE;
extern const char* XAP_GUID;
extern const char* XAP_DEFAULT_INSTANCE;

/********************************* xAP Handler & Parser ********************************/


#define HBEAT_INTERVAL 60

#define XAP_MSG_HBEAT  1		// special control message, type heartbeat
#define XAP_MSG_ORDINARY  2		// ordinary xap message type
#define XAP_MSG_CONFIG_REQUEST 3		// xap config message type
#define XAP_MSG_CACHE_REQUEST 4			// xap cache message type
#define XAP_MSG_CONFIG_REPLY 5		// xap config message type
#define XAP_MSG_CACHE_REPLY 6			// xap cache message type

#define XAP_MSG_UNKNOWN  0              // unknown xap message type
#define XAP_MSG_NONE 0                  // (or no message received)

#define XAP_MAX_MSG_ELEMENTS 50
#define XAP_MAX_KEYNAME_LEN 128
#define XAP_MAX_KEYVALUE_LEN 512
#define XAP_MAX_SECTION_LEN 128

#define DEBUG_OFF 0
#define DEBUG_INFO 1
#define DEBUG_VERBOSE 2
#define DEBUG_DEBUG 3


#define MAX_QUEUE_BACKLOG 50  // number of connections that can queue (ignored by OS I think?)
#define XAP_LOWEST_PORT 3639
#define XAP_HIGHEST_PORT 4639

struct tg_xap_msg {
     char section[XAP_MAX_SECTION_LEN+1];
     char name[XAP_MAX_KEYNAME_LEN+1];
     char value[XAP_MAX_KEYVALUE_LEN+1];
};

extern struct tg_xap_msg g_xap_msg[];

extern int g_debuglevel; // 0=off, 1=info, 2=verbose, 3=debug
extern int g_xap_index;

// xAP global socket structures
extern int g_xap_sender_sockfd;
extern int g_xap_receiver_sockfd;

// The network interface, port that xAP is to use.
extern char g_interfacename[20];
extern int g_interfaceport;
extern char g_ip[16];

extern char g_instance[20];
extern char g_serialport[20];
extern char g_uid[9];

extern struct sockaddr_in g_xap_receiver_address;
extern struct sockaddr_in g_xap_mybroadcast_address;
extern struct sockaddr_in g_xap_sender_address;

// From xaptx
int xap_send_tick(int a_interval);
int xap_broadcast_heartbeat(int a_sock,  struct sockaddr_in* a_addr);
int xap_send_message(const char* a_buff);
int xap_heartbeat_tick(int a_interval);
char *normalize_xap(char *body);

// From xaprx
int xap_compare(const char* a_item1, const char* a_item2);
int xap_poll_incoming(int a_server_sockfd, char* a_buffer, int a_buffer_size);
int xapmsg_parse(const unsigned char* a_buffer);
int xapmsg_getvalue(const char* a_keyname, char* a_keyvalue);
int xapmsg_updatevalue(const char* a_keyname, const char* a_keyvalue);
int xapmsg_toraw(char* a_raw_msg);
int xapmsg_gettype();

// From xapinit
int xap_discover_broadcast_network(int* a_sender_sockfd, struct sockaddr_in* a_sender_address);
int xap_discover_hub_address(int* a_receiver_sockfd, struct sockaddr_in* a_receiver_address, int a_port_range_low, int a_port_range_high);
int xap_init(int argc, char* argv[], int base);

#endif
