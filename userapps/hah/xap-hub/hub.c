/* xAP Compliant Hub

   Last revision: 14 Dec 02


   Usage: xap-hub [<interface>] [<port>] [<debug level>]

  e.g. xap-hub eth0 3333
		which uses interface eth0, port 3333
   or   xap=hub eth1
        which uses interface eth1, and defaults to port 2222
   or   xap=hub
	    which uses default interface eth0 and defaults to port 2222

   <Debug level> = 0-off; 1-info; 2-verbose; 3-debug
   
   Copyright (c) Patrick Lidstone, 2002.

   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  

   Please report defects to patrick@lidstone.net

   Compile with supplied makefile
 */

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
#ifndef WIN32
#include <net/route.h>
#endif
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "xapdef.h"
#include "xapGlobals.h"
#include "appdef.h"

char g_uid[9];

#define XAP_MAX_HUB_ENTRIES	50

struct tg_xap_hubentry {
	int port; // ip-port to forward to
	int interval;
	int timer;
	int is_alive;
};

struct tg_xap_hubentry g_xap_hubentry[XAP_MAX_HUB_ENTRIES];

void xaphub_build_heartbeat(char* a_buff, int a_port, const char* a_instance) {
	sprintf(a_buff, "xap-hbeat\n{\nv=12\nhop=1\nuid=%s\nclass=xap-hbeat.alive\nsource=%s.%s.%s\ninterval=%d\nport=%d\n}\n", g_uid, XAP_ME, XAP_SOURCE,  a_instance, HBEAT_INTERVAL, a_port);
		
	if (g_debuglevel>=DEBUG_VERBOSE) {
		printf("Heartbeat source=%s, instance=%s, interval=%d, port=%d\n",XAP_SOURCE, a_instance, HBEAT_INTERVAL, a_port );
	}
}

int xaphub_broadcast_heartbeat(const char* a_buff, int a_sock, const struct sockaddr_in* a_addr) {

	int i;

    i= sendto(a_sock, a_buff, strlen(a_buff), 0, (struct sockaddr *) &a_addr, sizeof(struct sockaddr_in));


	if (g_debuglevel>=DEBUG_VERBOSE) {
		printf("Broadcasting heartbeat\n");
	}
	return i;
}


int xaphub_init() {
	int i;
	for (i=0; i<XAP_MAX_HUB_ENTRIES; i++) {
			g_xap_hubentry[i].is_alive=0;
	}
	return 1;
}

int xaphub_addentry(int a_port, int a_interval) {
	// add new or update existing hub entry, resetting countdown timer
	int i;
	int matched;

	if(g_debuglevel) printf("xaphub_addentry %d %d\n", a_port, a_interval);
	matched=XAP_MAX_HUB_ENTRIES;


	for (i=0; i<XAP_MAX_HUB_ENTRIES; i++) {
		if (g_xap_hubentry[i].port==a_port) {
				// entry exists, update it
				g_xap_hubentry[i].interval=a_interval;
				g_xap_hubentry[i].timer=a_interval*2;
				g_xap_hubentry[i].is_alive=1;
				matched=i;
				if (g_debuglevel>=DEBUG_VERBOSE) {
				printf("Heartbeat for port %d\n",g_xap_hubentry[i].port);
				}
				break;
		}
	}
	if (matched==XAP_MAX_HUB_ENTRIES) {
		// no entry exists, create a new entry in first free slot
			for (i=0; i<XAP_MAX_HUB_ENTRIES; i++) {
				if (g_xap_hubentry[i].is_alive==0) {
						// free entry exists, use it it
						g_xap_hubentry[i].port=a_port;
						g_xap_hubentry[i].interval=a_interval;
						g_xap_hubentry[i].timer=a_interval*2;
						g_xap_hubentry[i].is_alive=1;
						matched=i;
						if (g_debuglevel>=DEBUG_INFO) {
						printf("Connecting port %d\n",g_xap_hubentry[i].port);
						}
						break;
				}
			}
	}
	return matched; // value of XAP_MAX_HUB_ENTRIES indicates list full

}

void xaphub_tick(time_t a_interval) {
	// Called regularly. a_interval specifies number of whole seconds
	// elapsed since last call.
	int i;

	for (i=0; i<XAP_MAX_HUB_ENTRIES; i++) {
		if ((g_xap_hubentry[i].is_alive)&&(g_xap_hubentry[i].timer!=0)) {
				// update timer entries. If timer is 0 then
				// ignore hearbeat ticks
				g_xap_hubentry[i].timer-=a_interval;

				if (g_xap_hubentry[i].timer<=0) {
					if (g_debuglevel>=DEBUG_INFO) {
					printf("Disconnecting port %d due to loss of heartbeat\n",g_xap_hubentry[i].port);
					}
					g_xap_hubentry[i].is_alive=0; // mark as idle
				}
				break;
		}
	}

}

int xaphub_relay(int a_txsock, const char* a_buf) {

	struct sockaddr_in tx_addr;
	int i,j=0;

	tx_addr.sin_family = AF_INET;		
	tx_addr.sin_addr.s_addr=inet_addr("127.0.0.1");


	for (i=0; i<XAP_MAX_HUB_ENTRIES; i++) {
		
		if (g_xap_hubentry[i].is_alive==1) {
				// entry exists, use it
			if (g_debuglevel>=DEBUG_VERBOSE) {
				printf("Relayed to %d\n",g_xap_hubentry[i].port);
				
			}
			j++;
			tx_addr.sin_port=htons(g_xap_hubentry[i].port);
	                sendto(a_txsock, a_buf, strlen(a_buf), 0, (struct sockaddr*)&tx_addr, sizeof(struct sockaddr_in));	
		}
	}
	return j; // number of connected hosts we relayed to
}


/* receive a xap (broadcast) packet and process */
int xap_handler(const char* a_buf, int a_port)
{

  char i_interval[16];
  char i_port[16];


  xapmsg_parse(a_buf);

  if (xapmsg_getvalue("xap-hbeat:interval", i_interval)==0) {
       if (g_debuglevel>=DEBUG_DEBUG) {
	    printf("Could not find <%s> in message\n","xap-hbeat.interval"); 
       }
  } else  {
       if (xapmsg_getvalue("xap-hbeat:port", i_port)==0) {
	    if (g_debuglevel>=DEBUG_DEBUG) {
		 printf("Could not find <%s> in message\n","xap-hbeat:port"); 
	    }
	    
       } else {
	    
	    xaphub_addentry(atoi(i_port), atoi(i_interval));
       }
  } // if interval
  
  return 1;
}

 
int main(int argc, char* argv[]) {

#define MAX_BACKLOG_QUEUE 20  // number of connections that can queue (ignored by OS I think?)
	
  struct protoent *ptrp;

  char buff[1500];
  char i_heartbeat_msg[1500];
  
  int server_sockfd;
  int client_len;
  
  struct sockaddr_in client_address;
  struct sockaddr_in server_address;

  struct sockaddr_in* p_server_address;
  
  int tx_sock;

  int i; 

  time_t i_timenow;
  time_t i_xaptick;
  time_t i_heartbeattick;

  long int i_inverted_netmask;

  struct ifreq i_interface;

  struct sockaddr_in  i_mybroadcast;
  struct sockaddr_in i_myinterface;
  struct sockaddr_in i_mynetmask;

  char i_interfacename[20];
  int i_interfaceport;

  int i_heartbeat_sock;
  struct sockaddr_in i_heartbeat_addr;
  int optval;
  int optlen;

  char i_instancename[40];

int i_optval, i_optlen;

  fd_set i_rdfs;
  struct timeval i_tv;
  int i_retval;
  
  xaphub_init();

  ptrp = getprotobyname("udp");  

 // Header verbage
 printf("\nxAP Hub Network Connector for xAP v1.2\n");
 printf("Copyright (C) Patrick Lidstone, 2002\n");

 if (argc<2) {
	 // set default interface
	 strcpy(i_interfacename, "eth0");
 } else
 {
	// get chosen interface
	 strncpy(i_interfacename, argv[1], sizeof(i_interfacename)-1);
 }

if (argc<3) {
	 // set default interface port
	 i_interfaceport=3639; 
 } else
 {
	// get chosen interface port
	 i_interfaceport=atoi(argv[2]);
 }

 if (argc<4) {
	 // set default debug level
	 g_debuglevel=DEBUG_OFF;
 } else
 {
	 g_debuglevel=atoi(argv[3]);
 }

//#################################################################################


//	int i; 
//	long int i_inverted_netmask;
//	struct ifreq i_interface;
//	struct sockaddr_in  i_mybroadcast;
//	struct sockaddr_in i_myinterface;
//	struct sockaddr_in i_mynetmask;
//	int i_optval, i_optlen;

	// Discover the broadcast network settings
	server_sockfd=socket(AF_INET, SOCK_DGRAM, 0);

	i_optval=1;
	i_optlen=sizeof(int);
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&i_optval, i_optlen)) {
		printf("Cannot set options on broadcast socket\n");
		return 0;
	}
	
	//setsockopt(any_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val) 

	i_optval=1;
	i_optlen=sizeof(int);
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&i_optval, i_optlen)) {
		printf("Cannot set reuseaddr options on broadcast socket\n");
		return 0;
	}

	// Query the low-level capabilities of the network interface
	// we are to use. If none passed on command line, default to
	// eth0.

	memset((char*)&i_interface, sizeof(i_interface),0);

	i_interface.ifr_addr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i_interface.ifr_addr.sa_family = AF_INET; 
	i=ioctl(server_sockfd, SIOCGIFADDR, &i_interface);

	if (i!=0) {
		printf("Could not determine interface address for interface %s\n",i_interfacename);
		return 0;
	}

	i_myinterface.sin_addr.s_addr=((struct sockaddr_in*)&i_interface.ifr_broadaddr)->sin_addr.s_addr;
	
	
	printf("%s: address %s\n",i_interface.ifr_name, inet_ntoa( ((struct sockaddr_in*)&i_interface.ifr_addr)->sin_addr));

	i_interface.ifr_broadaddr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i_interface.ifr_addr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i=ioctl(server_sockfd, SIOCGIFNETMASK, &i_interface);

	if (i!=0) {
		printf("Unable to determine netmask for interface %s\n",i_interfacename);
		return 0;
	}

	server_address.sin_port=htons(i_interfaceport);
  	server_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Non-blocking listener
  	fcntl(server_sockfd, F_SETFL, O_NONBLOCK);


	// First atttempt to open the a broadcast port
	// If this fails then we can assume that a hub is active on this host

	server_address.sin_family = AF_INET; 	
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);
	server_address.sin_port=htons(i_interfaceport);
		
	if (bind(server_sockfd, (struct sockaddr*)&server_address, sizeof(server_address))!=0) {
		printf("Broadcast socket port %d in use \n",i_interfaceport);
		exit(-1);
	}


	else {
		printf("Acquired broadcast socket, port %d\n",i_interfaceport);
		printf("Assuming no local hub is active\n");		
	}


	tx_sock=socket(AF_INET, SOCK_DGRAM, 0);
	listen(server_sockfd, MAX_BACKLOG_QUEUE);

  // Set up OUR heartbeat socket, on which we tell the world
  // we are alive and well
  
  	// Query the low-level capabilities of the network interface
	// we are to use. If none passed on command line, default to
	// eth0.

	memset((char*)&i_interface, sizeof(i_interface),0);

	i_interface.ifr_addr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i_interface.ifr_addr.sa_family = AF_INET; 
	i=ioctl(tx_sock, SIOCGIFADDR, &i_interface);

	if (i!=0) {
		printf("Could not determine interface address for interface %s\n",i_interfacename);
		exit(-1);
	}

	i_myinterface.sin_addr.s_addr=((struct sockaddr_in*)&i_interface.ifr_broadaddr)->sin_addr.s_addr;
	
	
	printf("%s: address %s\n",i_interface.ifr_name, inet_ntoa( ((struct sockaddr_in*)&i_interface.ifr_addr)->sin_addr));

	i_interface.ifr_broadaddr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i_interface.ifr_addr.sa_family = AF_INET; 
	strcpy(i_interface.ifr_name,i_interfacename);

	i=ioctl(tx_sock, SIOCGIFNETMASK, &i_interface);

	if (i!=0) {
		printf("Unable to determine netmask for interface %s\n",i_interfacename);
		exit(-1);
	}

	i_mynetmask.sin_addr.s_addr=((struct sockaddr_in*)&i_interface.ifr_broadaddr)->sin_addr.s_addr;
	i_inverted_netmask=~i_mynetmask.sin_addr.s_addr;
	i_mybroadcast.sin_addr.s_addr=i_inverted_netmask|i_myinterface.sin_addr.s_addr;
	
	printf("%s: netmask %s\n",i_interface.ifr_name, inet_ntoa( ((struct sockaddr_in*)&i_interface.ifr_netmask)->sin_addr));


	i_heartbeat_addr.sin_family=AF_INET;
	i_heartbeat_addr.sin_port=htons(i_interfaceport);
	i_heartbeat_addr.sin_addr.s_addr=i_mybroadcast.sin_addr.s_addr;

	i_heartbeat_sock =socket(AF_INET, SOCK_DGRAM,0);
	if (i_heartbeat_sock==-1) {
		printf("Unable to establish heartbeat broadcast socket on %s:%d\n",inet_ntoa(i_heartbeat_addr.sin_addr),i_interfaceport);
		exit(-1);
	}

    optval=1;
    optlen=sizeof(int);
    if (setsockopt(i_heartbeat_sock, SOL_SOCKET, SO_BROADCAST, (char*)&optval, optlen)) {
	   printf("Unable to set heartbeat broadcast socket options on %s:%d\n",inet_ntoa(i_heartbeat_addr.sin_addr),i_interfaceport);
	   exit(-1);
	}

	// Set xap-header.instance name for heartbeat
	// as "Linux-IPAddr"
	strcpy(i_instancename,"linux-");
	strcat(i_instancename,inet_ntoa(i_myinterface.sin_addr));
	
  // Parse heartbeat messages received on broadcast interface
  // If they originated from this host, add the port number to
  // the list of known ports
  // Otherwise ignore.
  // If ordinary header then pass to all known listeners
  
  i_xaptick=time((time_t*)0);
  i_heartbeattick=time((time_t*)0)-HBEAT_INTERVAL; // force heartbeat on startup

  printf("Ready\n");

  while (1==1) 
  { 
		i_timenow=time((time_t*)0);

	
		if (i_timenow-i_xaptick>=1) {
			if (g_debuglevel>=DEBUG_DEBUG) {
				printf("XAP Connection list tick %d\n",(int)i_timenow-(int)i_xaptick);
			}
			xaphub_tick(i_timenow-i_xaptick);
			i_xaptick=i_timenow;
		}

		if (i_timenow-i_heartbeattick>=HBEAT_INTERVAL) {
			if (g_debuglevel>=DEBUG_DEBUG) {
				printf("Outgoing heartbeat tick %d\n",(int)i_timenow-(int)i_heartbeattick);
			
			}
 			i_heartbeattick=i_timenow;

			xaphub_build_heartbeat(i_heartbeat_msg, i_interfaceport, i_instancename);

			// Send heartbeat to all external listeners
			xaphub_broadcast_heartbeat(i_heartbeat_msg, i_heartbeat_sock, &i_heartbeat_addr); 
			// Send heartbeat to all locally connected apps
			xaphub_relay(tx_sock, i_heartbeat_msg);


		} // heartbeat tick

	FD_ZERO(&i_rdfs);
	FD_SET(server_sockfd, &i_rdfs);
	
	i_tv.tv_sec=HBEAT_INTERVAL;
	i_tv.tv_usec=0;
	
	i_retval=select(server_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);
	// Select either timed out, or there was data - go look for it.
	client_len=sizeof(struct sockaddr);
	i=recvfrom(server_sockfd, buff, sizeof(buff), 0, (struct sockaddr*) &client_address, &client_len);
	
	
										
	if (i!=-1) {
		buff[i]='\0';
		
		if (g_debuglevel>=DEBUG_VERBOSE) {
			printf("Message from client %s:%d\n",inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port));
		}

			if (client_address.sin_addr.s_addr == i_myinterface.sin_addr.s_addr) {
				if (g_debuglevel>=DEBUG_VERBOSE) {
				printf("Message originated locally\n");
				}
				xap_handler(buff,ntohs(client_address.sin_port));

				// Either relay the heartbeat to all local apps, because
				// this was a heartbeat message - in which case the originator
				// will see his own heartbeat (but that doesn't really matter

				// or this wasn't a heartbeat message anyway, so we should
				// relay it. Again the originator will see his own message

				xaphub_relay(tx_sock, buff);

			}
			else {
				if (g_debuglevel>=DEBUG_VERBOSE) {
				printf("Message originated remotely, relay\n");
				}
				xaphub_relay(tx_sock, buff);
			}
		}
        
   }	// while        
}        // main


