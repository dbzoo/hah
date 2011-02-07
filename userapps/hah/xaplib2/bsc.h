/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <time.h>
#include "xap.h"

#define BSC_BINARY 0
#define BSC_LEVEL 1
#define BSC_STREAM 2

#define BSC_INPUT 0
#define BSC_OUTPUT 1

#define BSC_STATE_OFF 0
#define BSC_STATE_ON 1
#define BSC_STATE_UNKNOWN 3

#define BSC_INFO_CLASS "xAPBSC.info"
#define BSC_EVENT_CLASS "xAPBSC.event"
#define BSC_CMD_CLASS "xAPBSC.cmd"
#define BSC_QUERY_CLASS "xAPBSC.query"

#ifndef _BSC_H
#define _BSC_H

// An endpoint deails with section after the : in an xAP Address
// vendor,device.instance : name.subaddr
typedef struct _bscEndpoint {
	char *name;
	char *subaddr;
	char *id; // UID subaddress (2 hex digits)
	
	struct {
	   unsigned int io:1;  // input, output
	   unsigned int type:2; // binary, level, state
	   unsigned int state:2; // on, off, ?
	};
	union {
		char *level;
		char *text;
	};	

	void (*cmd)(struct _bscEndpoint *self);

	// Return 1 to emit an info/event, return 0 to deny
	int (*infoEvent)(struct _bscEndpoint *self, char *clazz);
	
	// Computed fields
	char *displayText;
	time_t last_report; // last time an INFO or EVENT was sent.
	char *uid;
	char *source; // vendor.device.instance:name.subaddr
	
	// User supplied object
	void *userData;

	struct _bscEndpoint *next;
} bscEndpoint;

bscEndpoint *bscAddEndpoint(bscEndpoint **head, char *name, char *subaddr, unsigned int dir, unsigned int typ,
                    void (*cmd)(struct _bscEndpoint *self),
                    int (*infoEvent)(struct _bscEndpoint *self, char *clazz)
                   );
void bscAddEndpointFilter(bscEndpoint *head, int info_interval);
void bscAddEndpointFilterList(bscEndpoint *head, int info_interval);
void *bscDelEndpoint(bscEndpoint *b);
void bscSetLevel(bscEndpoint *e, char *level);
void bscSetText(bscEndpoint *e, char *level);
void bscSetDisplayText(bscEndpoint *e, char *text);
void bscSetState(bscEndpoint *e, int state);
void bscSendCmdEvent(bscEndpoint *e);
bscEndpoint *bscFindEndpoint(bscEndpoint *head, char *name, char *subaddr);
int bscParseLevel(char *str);
char *bscStateToString(bscEndpoint *e);
char *bscIOToString(bscEndpoint *e);
int bscDecodeState(char *msg);
void bscSetEndpointUID(int nid);
int bscGetEndpointUID();
void bscFreeEndpointFilterList(bscEndpoint *head);

#endif
