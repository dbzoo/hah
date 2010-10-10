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
#include "utlist.h"

#define MAX_SOCKETS 5			// Additional sockets to listen on

#define MAX_QUEUE_BACKLOG 50
#define XAP_DATA_LEN 1500
#define XAP_MSG_ELEMENTS 50
#define XAP_HEARTBEAT_INTERVAL 60

extern const char *XAP_FILTER_ANY;
extern const char *XAP_FILTER_ABSENT;

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
	const char *section;
	const char *key;
	const char *value;
	struct _xAPFilter *next;
} xAPFilter;

struct _xAP;

typedef struct _xAPFilterCallback {
	void (*callback)(void *);
	void *user_data;
	xAPFilter *filter;
	struct _xAPFilterCallback *next;
} xAPFilterCallback;

typedef struct _xAPTimeoutCallback {
	void (*callback)(int, void *);
	void *user_data;
	int interval;
	time_t ttl;
	struct _xAPTimeoutCallback *next;
} xAPTimeoutCallback;

typedef struct _xAPSocketConnection {
	int fd;
	void *user_data;
	void (*callback)(int fd, void *data);
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

	char *ip;
	struct sockaddr_in txAddress;
	int txSockfd;

	unsigned char dataPacket[XAP_DATA_LEN];
	int parsedMsgCount;
	struct parsedMsgElement parsedMsg[XAP_MSG_ELEMENTS];

	xAPSocketConnection *connectionList;
	xAPTimeoutCallback *timeoutList;
	xAPFilterCallback *filterList;
} xAP;

// Global XAP object used by ALL - easier than constantly passing this about.
extern xAP *gXAP; 

// init.c
void xapInit(char *source, char *uid, char *interfaceName);
void xapInitFromINI(char *section, char *prefix, char *instance, char *uid, char *interfaceName, const char *inifile);
xAPSocketConnection *xapAddSocketListener(int fd, void (*callback)(int, void *), void *data);
xAPSocketConnection *xapFindSocketListenerByFD(int ifd);
void xapDelSocketListener(xAPSocketConnection **);
void simpleCommandLine(int argc, char *argv[], char **interfaceName);

// tx.c
void xapSend(const char *mess);
char *fillShortXap(char *shortMsg, char *uid, char *source);

// timeout.c
xAPTimeoutCallback *xapAddTimeoutAction(void (*func)(int, void *), int interval, void *data);
xAPTimeoutCallback *xapFindTimeoutByFunc(void (*func)(int, void*));
void timeoutDispatch();
void xapDelTimeoutAction(xAPTimeoutCallback **cb);
void xapDelTimeoutActionByFunc(void (*func)(int, void *));

// rx.c
void xapProcess();
void handleXapPacket(int fd, void *data);

//parse.c
int xapGetType();
char *xapGetValue(char *section, char *key);
int xapIsValue(char *section, char *key, char *value);
int parsedMsgToRaw(char *msg, int size);
int parsedMsgToRawWithoutSection(char *msg, int size, char *section);
int parseMsg();

//filter.c
int xapCompareFilters(xAPFilter *f);
xAPFilterCallback *xapAddFilterAction(void (*func)(void *), xAPFilter *filter, void *data);
void filterDispatch();
xAPFilter *xapAddFilter(xAPFilter **f, const char *section, const char *key, const char *value);
int xapFilterAddrSubaddress(char *filterAddr, char *addr);

// safe string copy
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

#endif
