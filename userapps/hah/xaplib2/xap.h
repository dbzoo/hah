/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifndef _XAP_H
#define _XAP_H

#include <time.h>
#include <arpa/inet.h>
#include "log.h"

#define MAX_SOCKETS 5			// Additional sockets to listen on

#define MAX_QUEUE_BACKLOG 50
#define XAP_DATA_LEN 1500
#define XAP_MSG_ELEMENTS 50
#define XAP_HEARTBEAT_INTERVAL 60

// Allow 100 sockets from default port.
#define XAP_PORT_L 3639
#define XAP_PORT_H 4639

#define XAP_MSG_HBEAT  1		// special control message, type heartbeat
#define XAP_MSG_ORDINARY  2		// ordinary xap message type
#define XAP_MSG_CONFIG_REQUEST 3	// xap config message type
#define XAP_MSG_CACHE_REQUEST 4		// xap cache message type
#define XAP_MSG_CONFIG_REPLY 5		// xap config message type
#define XAP_MSG_CACHE_REPLY 6		// xap cache message type
#define XAP_MSG_UNKNOWN  0              // unknown xap message type
#define XAP_MSG_NONE 0                  // (or no message received)

typedef struct _xAPFilter {
	char *section;
	char *key;
	char *value;
	struct _xAPFilter *next;
} xAPFilter;

struct _xAP;

typedef struct _xAPFilterCallback {
	void (*callback)(struct _xAP *, void *);
	void *user_data;
	xAPFilter *filter;
	struct _xAPFilterCallback *next;
} xAPFilterCallback;

typedef struct _xAPTimeoutCallback {
	void (*callback)(struct _xAP *, int, void *);
	void *user_data;
	int interval;
	time_t ttl;
	struct _xAPTimeoutCallback *next;
} xAPTimeoutCallback;

typedef struct _xAPSocketConnection {
	int fd;
	void *user_data;
	void (*callback)(struct _xAP*, int fd, void *data);
	struct _xAPSocketConnection *next;
} xAPSocketConnection;

struct parsedMsgElement {
	char *section;
	char *key;
	char *value;
};

typedef struct _xAP {
	char *source; // vendor.device.instance
	char *uid;

	int rxPort;
	int rxSockfd;

	struct sockaddr_in txAddress;
	int txSockfd;

	unsigned char dataPacket[XAP_DATA_LEN];
	int parsedMsgCount;
	struct parsedMsgElement parsedMsg[XAP_MSG_ELEMENTS];

	xAPSocketConnection *connectionList;
	xAPTimeoutCallback *timeoutList;
	xAPFilterCallback *filterList;
} xAP;

// init.c
xAP *xapNew(char *source, char *uid, char *interfaceName);
void xapAddSocketListener(xAP *xap, int fd, void (*callback)(xAP *, int, void *), void *data);

// tx.c
void xapSend(xAP *this, const char *mess);

// timeout.c
void xapAddTimeoutAction(xAP *this, void (*func)(xAP *, int, void *), int interval, void *data);
void timeoutDispatch(xAP *this);

// rx.c
void xapProcess(xAP *xap);
void handleXapPacket(xAP *xap, int fd, void *data);

//parse.c
int xapGetType(xAP *this);
char *xapGetValue(xAP *this, char *section, char *key);
int xapIsValue(xAP *this, char *section, char *key, char *value);
int parsedMsgToRaw(struct parsedMsgElement parsedMsg[], int parsedMsgCount, char *msg, int size);
int parseMsg(struct parsedMsgElement parsedMsg[], int maxParsedMsgCount, unsigned char *msg);

//filter.c
int xapCompareFilters(xAP *this, xAPFilter *f);
void xapAddFilterAction(xAP *this, void (*func)(xAP *, void *), xAPFilter *filter, void *data);
void filterDispatch(xAP *this);
void xapAddFilter(xAPFilter **f, char *section, char *key, char *value);
int xapFilterAddrSubaddress(char *filterAddr, char *addr);

// safe string copy
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

#endif
