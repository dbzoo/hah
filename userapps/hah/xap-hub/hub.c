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
char *interfaceName = NULL;

struct hubEntry {
	int port;
	int interval;
	int timer;
	int is_alive;	
	struct hubEntry *next;
} *hubList = NULL;

void hubRelay() {
	struct sockaddr_in tx_addr;
	struct hubEntry *entry;
	
	tx_addr.sin_family = AF_INET;
	tx_addr.sin_addr.s_addr=inet_addr("127.0.0.1");	

	debug("Tx xAP packet length %d\n%s", gXAP->frame.len, gXAP->frame.dataPacket);
	LL_FOREACH(hubList, entry) {
		// If its alive and we are not forward data back to it itself.
		if (entry->is_alive) {
			info("Relayed to %d",entry->port);
			tx_addr.sin_port=htons(entry->port);
			
			sendto(gXAP->txSockfd, gXAP->frame.dataPacket, gXAP->frame.len, 0, (struct sockaddr*)&tx_addr, sizeof(struct sockaddr_in));
		}
	}
	
}

/// Connect a new port or update an existing one with a heartbeat
void addOrUpdateHubEntry(int i_port, int i_interval) {
	debug("port %d interval %d", i_port, i_interval);
	struct hubEntry *entry;

	LL_SEARCH_SCALAR(hubList,entry,port,i_port);
	if(entry) { // Found entry
		info("Heartbeat for port %d", i_port);
	} else { // New entry
		entry = (struct hubEntry *)malloc(sizeof(struct hubEntry));
		entry->port = i_port;
		LL_PREPEND(hubList, entry);
		info("Connected port %d", i_port);
	}
	entry->interval = i_interval;
	entry->timer = i_interval*2;
	entry->is_alive = 1;	
}

/// Scan all hub connection and check if they are still alive.
void reapHubConnections(int interval, void *userData) {
	struct hubEntry *entry;
	LL_FOREACH(hubList, entry) {
		if (entry->is_alive) {
			entry->timer -= interval;
			if(entry->timer <= 0) {
				info("Disconnecting port %d due to loss of heartbeat", entry->port);
				entry->is_alive = 0;
			}
		}
	}	
}

void stopHubConnection(int port) {
	struct hubEntry *entry;
	LL_FOREACH(hubList, entry) {
		if (entry->port == port) {
		  info("Disconnecting port %d due xap-hbeat.stop", entry->port);
		  entry->is_alive = 0;
		  return;
		}
	}	
}

/// Rx socket handler callback
void xapRxBroadcast(int fd, void *userData) {
	// fd is gXAP->rxSockfd
	in_addr_t *myip = (in_addr_t *)userData;
	struct sockaddr_in clientAddr;
	
	int clientLen = sizeof(struct sockaddr);	
	gXAP->frame.len = recvfrom(gXAP->rxSockfd, gXAP->frame.dataPacket, XAP_DATA_LEN-1, 0, (struct sockaddr*) &clientAddr, (socklen_t *)&clientLen);
	if(gXAP->frame.len == 0) return; // Huh?  How did the select() do this if there is no data!?
	
	if (gXAP->frame.len < 0) {
		err_strerror("recvfrom");
		return;
	}

        info("Message from client %s:%d",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
	
	// (msglen > 0)
	// terminate the buffer so we can treat it as a conventional string
	gXAP->frame.dataPacket[gXAP->frame.len] = '\0';
	debug("Rx xAP packet\n%s", gXAP->frame.dataPacket);

	if(clientAddr.sin_addr.s_addr == *myip) {
		info("Message originated locally");
		xAPFrame f;
                // Use a local frame as Parsing will mess with the message we need to relay.
		memcpy(&f, &gXAP->frame, sizeof(xAPFrame));
		parseMsgF(&f);

		if(xapGetTypeF(&f) == XAP_MSG_HBEAT) {
			char *interval = xapGetValueF(&f,"xap-hbeat","interval");
			char *port = xapGetValueF(&f,"xap-hbeat","port");
			debug_if(interval == NULL,"Missing interval in heartbeat");
			debug_if(port == NULL,"Missing port in heartbeat");
			
			if(port && interval) {
				int iport = atoi(port);
				if(iport == gXAP->rxPort) {
					debug("Detected our own heartbeat not forwarding");
					return;
				}			

				char *class = xapGetValueF(&f,"xap-hbeat","class");
				if(class && strcmp(class,"xap-hbeat.stop") == 0) {
				  stopHubConnection(iport);
				} else {
				  addOrUpdateHubEntry(iport, atoi(interval));
				}
			}
		}
	} else {
		info("Message originated remotely");
	}
	hubRelay();
	
}

int main(int argc, char **argv) {
	printf("xAP HUB\n");
	printf("Copyright (C) DBzoo, 2008-2014\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);
	
	gXAP = (xAP *)calloc(sizeof(xAP), 1);
	
	discoverBroadcastNetwork(&gXAP->txAddress, &gXAP->txSockfd, &gXAP->ip, interfaceName);
	discoverHub(&gXAP->rxPort, &gXAP->rxSockfd, &gXAP->txAddress);
	die_if(gXAP->rxPort != XAP_PORT_L,"Port %d not available", XAP_PORT_L);

	gXAP->source = xapBuildAddress("dbzoo", NULL, "hub");
	gXAP->uid = "FF00DC00";

	xapAddTimeoutAction(&heartbeatHandler, XAP_HEARTBEAT_INTERVAL, NULL);	
	xapAddTimeoutAction(&reapHubConnections, reapTimeSecs, NULL);

	// Any data on our listening port is handled by xapRxBroadcast()
	in_addr_t myip = inet_addr(gXAP->ip);
	xapAddSocketListener(gXAP->rxSockfd, &xapRxBroadcast, (void *)&myip);
	
	xapProcess();
	return 0;
}
