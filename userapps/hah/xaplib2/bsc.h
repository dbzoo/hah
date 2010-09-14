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

#define STATE_OFF 0
#define STATE_ON 1
#define STATE_UNKNOWN 3

#define BSC_INFO 0
#define BSC_EVENT 1

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

	void (*cmd)(xAP *xap, struct _bscEndpoint *self);
	void (*infoEvent)(xAP *xap, struct _bscEndpoint *self, char *clazz);
	
	// Computed fields
	char *displayText;
	time_t last_report; // last time an INFO or EVENT was sent.
	char *uid;
	char *source; // vendor.device.instance:name.subaddr
	
	struct _bscEndpoint *next;
} bscEndpoint;

void bscAddEndpoint(bscEndpoint **head, char *name, char *subaddr, char *id, unsigned int dir, unsigned int typ,
                    void (*cmd)(xAP *xap, struct _bscEndpoint *self),
                    void (*infoEvent)(xAP *xap, struct _bscEndpoint *self, char *clazz)
                   );
void xapAddBscEndpointFilters(xAP *xap, bscEndpoint *head, int info_interval);
void bscInfoEvent(xAP *xap, bscEndpoint *e, char *clazz);
void setbscLevel(bscEndpoint *e, char *level);
void setbscText(bscEndpoint *e, char *level);
void setbscState(bscEndpoint *e, int state);
bscEndpoint *findbscEndpoint(bscEndpoint *head, char *name, char *subaddr);
int bscParseLevel(char *str);

#endif
