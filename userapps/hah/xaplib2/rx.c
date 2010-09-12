/* $Id$
*
* Copyright: See COPYING file that comes with this distribution
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include "xap.h"

static int readXapData(xAP *xap) {
	int i = recvfrom(xap->rxSockfd, xap->dataPacket, XAP_DATA_LEN-1, 0, 0, 0);
	if (i > 0) {
		 // terminate the buffer so we can treat it as a conventional string
		xap->dataPacket[i] = '\0';
	}
	return i;
}

void handleXapPacket(xAP *xap, int fd, void *data) {
	if(readXapData(xap) > 0) {
		xap->parsedMsgCount = parseMsg(xap->parsedMsg, XAP_MSG_ELEMENTS, xap->dataPacket);
		filterDispatch(xap);
	}
}

static int buildSelectList(xAPSocketConnection *list, fd_set *socks) {
	int highsock = -1;
	FD_ZERO(socks);
	while(list) {
		FD_SET(list->fd, socks);
		if (list->fd > highsock)
			highsock = list->fd;
		list = list->next;
	}
	return highsock;
}
// Check if data is present on the SOCKET if so dispatch to its handler.
static void readSockets(xAP *xap, fd_set *socks) {
	xAPSocketConnection *list = xap->connectionList;
	while(list) {
		if (FD_ISSET(list->fd, socks))
			(*list->callback)(xap, list->fd, list->user_data);
		list = list->next;
	}
}

void xapProcess(xAP *xap) {
	fd_set socks; // Socket file descriptors we want to wake up for, using select()
	int readsocks; // number of sockets ready for reading
	struct timeval timeout;  // Timeout for select()

	//heartbeatHandler(xap, XAP_HEARTBEAT_INTERVAL, NULL);
	while(xap) {
		int highsock = buildSelectList(xap->connectionList, &socks);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		readsocks = select(highsock+1, &socks, NULL, NULL, &timeout);
		
		if (readsocks < 0) {
			perror("select");
			exit(-1);
		}
		if (readsocks == 0) { // Nothing ready to read
			timeoutDispatch(xap);
		} else
			readSockets(xap, &socks);		
	}
}
