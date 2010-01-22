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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xapdef.h"
#include "serial.h"
#include "bridge.h"

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "Bridge";
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

int maxHopCount=5;

/* Assuming an xAP message has already been parsed, examine the HOP,
 * increment and return the new XAP message.
 */
static int incrementHop(char *xap) {
     int hop_count;
     char hop_count_str[3];

     // Ordinary class of xAP message
     xapmsg_getvalue("xap-header:hop", hop_count_str);
     hop_count = atoi(hop_count_str);
     if (hop_count > maxHopCount) {
	  debug(LOG_INFO, "Relay refused, hop count %d exceeded\n", maxHopCount);
	  return 0;
     }
     sprintf(hop_count_str,"%d",hop_count+1);
     xapmsg_updatevalue("xap-header:hop",hop_count_str);

     // After the HOP count is bumped update the mesg
     xapmsg_toraw(xap);

     return 1;
}

/*
** Adjust message HOP count and forward to the serial ports.
*/
void xap_handler(char *xap) {
     xapmsg_parse(xap);

     // We never forward HEARTBEATS to the serial devices.
     if (xapmsg_gettype() == XAP_MSG_HBEAT) return;

     if(incrementHop(xap) == 0) return;

     char *serial_msg = frameSerialXAPpacket(xap);
     portConf *pEntry;
     for (pEntry = pPortList; pEntry; pEntry = pEntry->pNext) {
	  if (pEntry->enabled) 
	       sendSerialMsg(pEntry, serial_msg);
     }
}


/*
** Handle an inbound xAP serial packet.  Sending to the Ethernet and
** relaying to other (non originator) serial devices.
*/
void serial_handler(portConf *pEntry) {
     char xap_ser[1500]; // Serial transport framed XAP packet.
     ssize_t r;

     r = read(pEntry->fd, xap_ser, sizeof(xap_ser));
     if (r < 0) {
	  debug(LOG_DEBUG,"readSerialMsg(): reading %s:%m", pEntry->devc);
	  return;
     }
/*
     if(g_debuglevel >= LOG_DEBUG ) {
	  printf("serial_handler(): read ");
	  xap_ser[r] = 0; // teminate for printing
	  ldump(xap_ser, r);
     }
*/
     char *xap = unframeSerialMsg(pEntry->serialST, xap_ser, r);
     if (xap) {
	  xapmsg_parse(xap);
	  if(incrementHop(xap) == 0) return;

	  xap_send_message(xap);

	  // Don't forward a serial devices HEARTBEAT to
	  // other serial devices.
	  if (xapmsg_gettype() == XAP_MSG_HBEAT) return;

	  // To serial devices
	  portConf *entry;
	  for (entry = pPortList; entry; entry = entry->pNext) {
	       if (entry->enabled && entry != pEntry)
		    sendSerialMsg(entry, xap_ser);
	  }
     }

}

/*
** Receive/Send message loop.
*/
void packetLoop() {
     fd_set rdfs, m_rdfs;
     struct timeval tv;
     portConf *pEntry;
     int highest_fd = g_xap_receiver_sockfd;
     int rv = 0;
     char xap_buff[1500];

     // Setup select() FD's and open each serial port.
     FD_ZERO(&m_rdfs);
     FD_SET(g_xap_receiver_sockfd, &m_rdfs);
     debug(LOG_DEBUG, "packetLoop(): UDP fd %d", g_xap_receiver_sockfd);
     for (pEntry = pPortList; pEntry; pEntry = pEntry->pNext) {
	  int fd = openSerialPort(pEntry);
	  if( fd != -1) {
	       debug(LOG_DEBUG, "packetLoop(): serial fd %d", fd);
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


     // MAIN LOOP.
     while(1) {
	  rdfs = m_rdfs;
	  tv.tv_sec = HBEAT_INTERVAL;
	  tv.tv_usec = 0;
	
	  rv = select(highest_fd, &rdfs, NULL, NULL, &tv);

	  if(rv == -1) {
	       debug(LOG_DEBUG,"packetLoop() select error: %m");
	  } else if (rv) {

	       // Inbound UDP XAP packet
	       if (FD_ISSET(g_xap_receiver_sockfd, &rdfs)) {	
		    if (xap_poll_incoming(g_xap_receiver_sockfd, xap_buff, sizeof(xap_buff))>0) {
			 xap_handler(xap_buff);
		    }
	       }
	       else  // A serial device input
	       {
		    for (pEntry = pPortList; pEntry; pEntry = pEntry->pNext) {
			 if(FD_ISSET(pEntry->fd, &rdfs)) {
			      serial_handler(pEntry);
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
     n = ini_gets("bridge","uid","00D8",uid,sizeof(uid),inifile);

     // validate that the UID can be read as HEX
     if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) &&
		    isxdigit(uid[2]) && isxdigit(uid[3])))
     {
	  debug(LOG_INFO,"Missing/Invalid UID %s in .INI file using default", uid);
	  strlcpy(uid,"00D8",sizeof uid);
     }
     snprintf(guid,sizeof guid,"FF%s00", uid);
     XAP_GUID = strdup(guid);

     char control[30];
     n = ini_gets("bridge","instance","",control,sizeof(control),inifile);
     if(n == 0 || strlen(control) == 0)
     {
	  char buf[80];
	  if(gethostname(buf, sizeof(buf)) == 0) {
	       strlcpy(control, buf, sizeof(control));
	  } else {
	       strlcpy(control, "one",sizeof control);
	  }
     }
     XAP_DEFAULT_INSTANCE = strdup(control);

     maxHopCount = ini_getl("bridge", "hopCount", 5, inifile);
     // HopCount of 1 could mean the bridge would drop every packet. -ve don't sense either.
     if (maxHopCount < 2) maxHopCount = 5;

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
	  debug(LOG_EMERG, "Quiting!! Error opening config file %s:%m", inifile);
	  exit(1);
     }
     packetLoop();
}
