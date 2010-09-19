/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include "xap.h"

/// Receive xAP packet data
static int readXapData(xAP *xap) {
	int i = recvfrom(xap->rxSockfd, xap->dataPacket, XAP_DATA_LEN-1, 0, 0, 0);
	if (i > 0) {
		 // terminate the buffer so we can treat it as a conventional string
		xap->dataPacket[i] = '\0';
		info("Rx xAP packet\n%s", xap->dataPacket);
	}
	return i;
}

/** Rx socket handler callback for registration.
* Registered via function xapAddSocketListener()
*/
void handleXapPacket(xAP *xap, int fd, void *data) {
	// If we have no filters then we don't care about xAP packets!
	// When the receive buffer overflows as we are using UDP the data will
	// just be dropped.
	if(xap->filterList == NULL) return;	
	if(readXapData(xap) > 0) {
		xap->parsedMsgCount = parseMsg(xap->parsedMsg, XAP_MSG_ELEMENTS, xap->dataPacket);
		filterDispatch(xap);
	}
}

/** Find the highest socket FD in the list of socket listeners.
* Returns: Socket FD or -1 if no sockets are registered.
*/
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
/// Check if data is present on the SOCKET if so dispatch to its handler.
static void readSockets(xAP *xap, fd_set *socks) {
	xAPSocketConnection *list = xap->connectionList;
	while(list) {
		if (FD_ISSET(list->fd, socks))
			(*list->callback)(xap, list->fd, list->user_data);
		list = list->next;
	}
}

/** xAP main processing loop.
*/
void xapProcess(xAP *xap) {
	fd_set socks; // Socket file descriptors we want to wake up for, using select()
	int readsocks; // number of sockets ready for reading
	struct timeval timeout;  // Timeout for select()

	while(xap) {
		int highsock = buildSelectList(xap->connectionList, &socks);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		readsocks = select(highsock+1, &socks, NULL, NULL, &timeout);
		
		if (readsocks < 0) {
			err_strerror("select()");
		}
		if (readsocks == 0) { // Nothing ready to read
			timeoutDispatch(xap);
		} else
			readSockets(xap, &socks);		
	}
}
