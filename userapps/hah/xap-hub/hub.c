/* $Id$
   Copyright (c) Brett England, 2010

   xAP HUB

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "xap.h"

const int reapTimeSecs = 10;
xAP *gXAP;
char *interfaceName = "eth0";

struct hubEntry {
	int port;
	int interval;
	int timer;
	int is_alive;	
	struct hubEntry *next;
} *hubList = NULL;

void hubRelay(int msglen) {
	struct sockaddr_in tx_addr;
	struct hubEntry *entry;
	
	tx_addr.sin_family = AF_INET;
	tx_addr.sin_addr.s_addr=inet_addr("127.0.0.1");	
	
	LL_FOREACH(hubList, entry) {
		// If its alive and we are not forward data back to it itself.
		if (entry->is_alive) {
			debug("Relayed to %d",entry->port);
			tx_addr.sin_port=htons(entry->port);
			
			sendto(gXAP->txSockfd, gXAP->dataPacket, msglen, 0, (struct sockaddr*)&tx_addr, sizeof(struct sockaddr_in));
		}
	}
	
}

/// Connect a new port or update an existing one with a heartbeat
void addOrUpdateHubEntry(int i_port, int i_interval) {
	debug("port %d interval %d", i_port, i_interval);
	struct hubEntry *entry;

	LL_SEARCH_SCALAR(hubList,entry,interval,i_interval);
	if(entry) { // Found entry
		debug("Heartbeat for port %d", i_port);
	} else { // New entry
		entry = (struct hubEntry *)malloc(sizeof(struct hubEntry));
		entry->port = i_port;
		LL_PREPEND(hubList, entry);
		debug("Connected port %d", i_port);
	}
	entry->interval = i_interval;
	entry->timer = i_interval*2;
	entry->is_alive = 1;	
}

/// Scan all hub connection and check if they are still alive.
void reapHubConnections(int interval, void *userData) {
	struct hubEntry *entry;
	LL_FOREACH(hubList, entry) {
		if (entry->is_alive && entry->timer) {
			entry->timer -= interval;
			if(entry->timer <= 0) {
				debug("Disconnecting port %d due to loss of heartbeat", entry->port);
				entry->is_alive = 0;
			}
		}
	}	
}

/// Rx socket handler callback
void xapRxBroadcast(int fd, void *userData) {
	// fd is gXAP->rxSockfd
	in_addr_t *myip = (in_addr_t *)userData;
	struct sockaddr_in clientAddr;
	
	int clientLen = sizeof(struct sockaddr);	
	int msglen = recvfrom(gXAP->rxSockfd, gXAP->dataPacket, XAP_DATA_LEN-1, 0, (struct sockaddr*) &clientAddr, (socklen_t *)&clientLen);
	if(msglen == 0) return; // Huh?  How did the select() do this if there is no data!?
	
	if (msglen < 0) {
		err_strerror("recvfrom");
		return;
	}

        debug("Message from client %s:%d",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
	
	// (msglen > 0)
	// terminate the buffer so we can treat it as a conventional string
	gXAP->dataPacket[msglen] = '\0';
	debug("Rx xAP packet\n%s", gXAP->dataPacket);

	if(clientAddr.sin_addr.s_addr == *myip) {
		debug("Message originated locally");
		parseMsg();

		if(xapGetType() == XAP_MSG_HBEAT) {
			char *interval = xapGetValue("xap-hbeat","interval");
			char *port = xapGetValue("xap-hbeat","port");
			debug_if(interval == NULL,"Missing interval in heartbeat");
			debug_if(port == NULL,"Missing interval in heartbeat");
			
			if(port && interval) {
				int iport = atoi(port);
				if(iport == gXAP->rxPort) {
					debug("Detected our own heartbeat not forwarding");
					return;
				}			
				addOrUpdateHubEntry(atoi(port), atoi(interval));
			}
		}
	} else {
		debug("Message originated remotely");
	}
	hubRelay(msglen);
	
}

int main(int argc, char **argv) {
	printf("xAP HUB\n");
	printf("Copyright (C) DBzoo, 2008-2010\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);
	
	gXAP = (xAP *)calloc(sizeof(xAP), 1);
	
	discoverBroadcastNetwork(&gXAP->txAddress, &gXAP->txSockfd, &gXAP->ip, interfaceName);
	discoverHub(&gXAP->rxPort, &gXAP->rxSockfd, &gXAP->txAddress);

	char source[64];
	sprintf(source, "dbzoo.hub.linux-%s", gXAP->ip);
	gXAP->source = strdup(source);
	gXAP->uid = "FF00DC00";

	
	xapAddTimeoutAction(&heartbeatHandler, XAP_HEARTBEAT_INTERVAL, NULL);	
	xapAddTimeoutAction(&reapHubConnections, reapTimeSecs, NULL);

	// Any data on our listening port is handled by xapRxBroadcast()
	in_addr_t myip = inet_addr(gXAP->ip);
	xapAddSocketListener(gXAP->rxSockfd, &xapRxBroadcast, (void *)&myip);
	
	xapProcess();
	return 0;
}
