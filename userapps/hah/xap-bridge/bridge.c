/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include "xapdef.h"
#include "serial.h"
#include "bridge.h"
#include "queue.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE;
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE = "Bridge";

static int maxHopCount=5;
static int forwardHeartbeats=0;  // forward heart-beats to serial networks.
static struct queue *tx_queue;
static pthread_mutex_t tx_udp = PTHREAD_MUTEX_INITIALIZER;

struct xapPacket {
     char *xap;         // xAP message (serial unframed).
     portConf *pDevice; // NULL for ethernet.
};

/* Store a XAP packet to the FIFO queue for processing in the txLoop()
*/
void pushPacket(char *xap, portConf *pEntry) {
     struct xapPacket *r;

     r = mem_malloc(sizeof(struct xapPacket), M_ZERO);
     r->xap = mem_strdup(xap, M_NONE);
     r->pDevice = pEntry;
     
     queue_push(tx_queue, r);
}

/* Push an inbound UDP packet to the FIFO Tx queue.
*/
static inline void rx_udp_handler(char *xap) {
     debug(LOG_INFO,"rx_udp_handler(): queue UDP packet");
     pushPacket(xap, NULL);     
}

/* Push an inbound serial packet to the FIFO Tx queue.
*/
static void rx_serial_handler(portConf *pEntry) {
     char xap_in[1500]; // Serial transport framed XAP packet.
     ssize_t r;

     r = read(pEntry->fd, xap_in, sizeof(xap_in));
     if (r < 0) {
	  debug(LOG_DEBUG,"readSerialMsg(): reading %s:%m", pEntry->devc);
	  return;
     }

     if(g_debuglevel == LOG_DEBUG ) {
	  printf("serial_handler(): %s ", pEntry->devc);
	  ldump(xap_in, r);
     }

     char *rawxap = unframeSerialMsg(pEntry->serialST, xap_in, r);
     if (rawxap) { // Do we have a complete message yet?
	  debug(LOG_INFO,"rx_serial_handler(): queue SERIAL packet from %s", pEntry->devc);
	  pushPacket(rawxap, pEntry);
     }
}


/** Examine the HOP increment and return the new XAP message
    into the message area passed in.
*/
static int incrementHop(char *xap) {
     int hop_count;
     char hop_count_str[3];

     // Ordinary class of xAP message
     xapmsg_parse(xap);
     xapmsg_getvalue("xap-header:hop", hop_count_str);
     hop_count = atoi(hop_count_str);

     // NOTE: If the hop is greater the 9 we can't push the message
     // back into the same memory location otherwise we may exceed the
     // char *xap memory bounds: xapmsg_raw() may segv.
     // We can live with a 9 hop limit.
     if(hop_count > 9) {
	  debug(LOG_ERR, "Cannot exceed more then 9 hops");
	  return 0;
     }

     if (hop_count > maxHopCount) {
	  debug(LOG_NOTICE, "Relay refused, hop count %d exceeded\n", maxHopCount);
	  return 0;
     }
     sprintf(hop_count_str,"%d",hop_count+1);
     xapmsg_updatevalue("xap-header:hop",hop_count_str);

     // After the HOP count is bumped update the mesg
     xapmsg_toraw(xap);

     return 1;
}

static void *txLoop() {
     struct xapPacket *d;

     for(;;) {
	  // Read data from Tx QUEUE - blocking.
	  d = queue_pop(tx_queue);
	  debug(LOG_INFO, "txLoop(): Read from queue %x", d->xap);

	  // d->xap is altered with bumped hop as side-effect.
	  if(incrementHop(d->xap) == 0) goto exit;

	  // If serial originated send to Ethernet.
	  if(d->pDevice) {
	       debug(LOG_INFO,"txLoop(): send to Ethernet");
	       xap_send_message(d->xap);
	  }

	  // Forward a HEARTBEAT to serial devices?
	  if (forwardHeartbeats == 0 && xapmsg_gettype() == XAP_MSG_HBEAT) goto exit;

	  // To serial devices
	  portConf *entry;
	  char *xap = NULL; // Framed XAP payload for serial transmission
	  for (entry = pPortList; entry; entry = entry->pNext) {
	       if(entry == d->pDevice) continue; // Exclude the originator
	       if (entry->enabled && entry->xmit.tx) { 
		    if (xap == NULL) { // Lazily frame packet for serial Xmit.
			 xap = frameSerialXAPpacket(d->xap);
		    }
		    debug(LOG_INFO,"txLoop(): send to %s", entry->devc);
		    sendSerialMsg(entry, xap);
	       }
	  }	  
     exit:
	  mem_free(d->xap);
	  mem_free(d);
     }
     pthread_exit(NULL); /* never reached */
}

/*
** Receive message loop.
*/
void rxLoop() {
     fd_set rdfs, m_rdfs;
     struct timeval tv;
     portConf *pEntry;
     int highest_fd = g_xap_receiver_sockfd;
     int rv = 0;
     char xap_buff[1500];

     // Setup select() FD's and open each serial port.
     FD_ZERO(&m_rdfs);
     FD_SET(g_xap_receiver_sockfd, &m_rdfs);
     for (pEntry = pPortList; pEntry; pEntry = pEntry->pNext) {
	  int fd = openSerialPort(pEntry);
	  if( fd != -1 && pEntry->xmit.rx) {
	       debug(LOG_DEBUG, "packetLoop(): serial Rx %s", pEntry->devc);
	       FD_SET(fd, &m_rdfs);
	       if (fd > highest_fd) highest_fd = fd;
	       rv++;
	  }
     }
     if (rv == 0) {
	  debug(LOG_EMERG, "Quiting!! Failed to open any serial ports");
	  exit(1);	  
     }
     highest_fd++;

     for(;;) {
	  rdfs = m_rdfs;
	  tv.tv_sec = HBEAT_INTERVAL;
	  tv.tv_usec = 0;

	  debug(LOG_DEBUG,"rxLoop(): select");
	  rv = select(highest_fd, &rdfs, NULL, NULL, &tv);
  
	  if(rv < 0) {
	       debug(LOG_DEBUG,"packetLoop() select error: %m");
	  } else if (rv) {
	       // Inbound UDP XAP packet
	       if (FD_ISSET(g_xap_receiver_sockfd, &rdfs)) {	
		    if (xap_poll_incoming(g_xap_receiver_sockfd, xap_buff, sizeof(xap_buff))>0) {
			 rx_udp_handler(xap_buff);
		    }
	       }
	       else  // A serial device input
	       {
		    for (pEntry = pPortList; pEntry; pEntry = pEntry->pNext) {
			 if(FD_ISSET(pEntry->fd, &rdfs)) {
			      rx_serial_handler(pEntry);
			      break;
			 }
		    }

	       }	
	  } // else Timeout

	  xap_heartbeat_tick(HBEAT_INTERVAL);	
     }
}

static void setupXAP() {
     char uid[5];
     char guid[9];
     long n;

     // default to 00DC if not present
     n = ini_gets("bridge","uid","00D3",uid,sizeof(uid),inifile);

     // validate that the UID can be read as HEX
     if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) &&
		    isxdigit(uid[2]) && isxdigit(uid[3])))
     {
	  debug(LOG_INFO,"Missing/Invalid UID %s in .INI file using default", uid);
	  strlcpy(uid,"00D3",sizeof uid);
     }
     snprintf(guid,sizeof guid,"FF%s00", uid);
     XAP_GUID = strdup(guid);

     char i_control[64];
     char s_control[128];
     n = ini_gets("xap","instance","",i_control,sizeof(i_control),inifile);
     strcpy(s_control, "livebox");
     // If there a unique HAH sub address component?
     if(i_control[0]) {
	     strlcat(s_control, ".",sizeof(s_control));
	     strlcat(s_control, i_control, sizeof(s_control));
     }
     XAP_SOURCE = strdup(s_control);
     strlcpy(g_instance, XAP_DEFAULT_INSTANCE, sizeof g_instance); // default instance name

     maxHopCount = ini_getl("bridge", "hopCount", 5, inifile);
     // HopCount of 1 could mean the bridge would drop every packet. -ve don't sense either.
     if (maxHopCount < 2) maxHopCount = 5;

     forwardHeartbeats = ini_getl("bridge", "forwardHeartbeats", 0, inifile);

     strlcpy(g_uid, XAP_GUID, sizeof g_uid);

     xap_discover_broadcast_network(&g_xap_sender_sockfd, &g_xap_sender_address);
     memcpy(&g_xap_mybroadcast_address, &g_xap_sender_address, sizeof(g_xap_sender_address));
     xap_discover_hub_address(&g_xap_receiver_sockfd, &g_xap_receiver_address, XAP_LOWEST_PORT, XAP_HIGHEST_PORT);
}

static void usage() {
     printf("xap-bridge [options]\n");
     printf("\n");
     printf("   Options:\n");
     printf("      -c <file>   Configuration file, def: %s\n", inifile);
     printf("\n");
     printf("      -d <dbg>    Set debug message\n");
     printf("\n");
     printf("      -i          Interface, def: %s\n", g_interfacename);
}

int main(int argc, char *argv[]) {
     int opt;

     printf("\nSerial Bridge for xAP v12\n");
     printf("Copyright (C) DBzoo, 2010\n\n");

     g_interfaceport=3639;
     strcpy(g_interfacename, "br0");

     while ((opt = getopt(argc, argv, "c:d:i:h?")) != -1)
     {
	  switch (opt)
	  {
	  case 'c':
	       strcpy(inifile, optarg);
	       break;
	  case 'd':
	       g_debuglevel = atoi(optarg);
	       break;	
	  case 'i':
	       strlcpy(g_interfacename, optarg, sizeof(g_interfacename));
	       break;
	  case 'h':
	  case '?':
	  default:
	       usage();
	       exit(1);
	       break;
	  }
     }

     setupXAP();

     if (loadConfig() < 1)
     {
	  debug(LOG_EMERG, "Quiting! Config file %s:%m", inifile);
	  exit(1);
     }

     if ((tx_queue = queue_new()) == NULL) {
	  debug(LOG_EMERG, "main(): Fail to init FIFO queue");
	  exit(1);	  
     }

     pthread_t tx_thread;     
     int rv = pthread_create(&tx_thread, NULL, txLoop, NULL);
     if(rv) {
	  debug(LOG_EMERG, "main(): pthread_create() ret %d:%m", rv);
	  exit(1);
     }

     rxLoop();
    
     exit(0);  /* never reached */
}
