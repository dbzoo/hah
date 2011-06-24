/* $Id$
   Copyright (c) Brett England, 2010

   Demonstration usage for the xAP and BSC library calls.

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xap.h"
#include "bsc.h"

#define INFO_INTERVAL 120

xAP *gXAP = NULL;  // Usage of the library requires a global XAP object.

/// Handle xapBSC.cmd for the RELAY endpoints.
void cmdRelay (bscEndpoint *e)
{
        char buf[30];
	snprintf(buf, sizeof(buf), "relay %s %s\n", e->subaddr, bscStateToString(e));
        printf(buf);
}

/// Handle xapBSC.cmd for the LCD endpoint.
void cmdLCD(bscEndpoint *e)
{
        char buf[30];
        snprintf(buf, sizeof(buf), "lcd %s\n", e->text);
        printf(buf);
}

/// Augment default xapBSC.info & xapBSC.event handler.
int infoEvent1wire (bscEndpoint *e, char *clazz)
{
        // Lazy memory allocation of displayText
        if(e->displayText == NULL)
                e->displayText = (char *)malloc(20);
        // Configure the displayText optional argument.
        snprintf(e->displayText, 20, "Temperature %s C", e->text);
        return 1;
}

/// A timeout callback
void timeout(int interval, void *data) {
	char *s = (char *)data;
	time_t now = time(NULL);
	char *t = ctime(&now);
	t[strlen(t)-1] = 0;
	printf("%s - Interval %d - %s\n", t, interval, s);
}

int main(int argc, char *argv[])
{
	setLoglevel(LOG_INFO);
	xapInit("dbzoo.livebox.Demo","FF00DB00", "eth0");
	die_if(gXAP == NULL,"Failed to init xAP");
	
	bscEndpoint *ep = NULL;
        // An INPUT can't have a command callback, if supplied it'd be ignored anyway.
        // A NULL infoEvent function is equiv to supplying bscInfoEvent().
	// Params: LIST, NAME, SUBADDR, UID, IO, DEVICE TYPE, CMD CALLBACK, INFO/EVENT CALLBACK
	bscAddEndpoint(&ep, "1wire", "1", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
        bscAddEndpoint(&ep, "1wire", "2", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
        bscAddEndpoint(&ep, "input", "1", BSC_INPUT, BSC_BINARY, NULL, NULL);
        bscAddEndpoint(&ep, "input", "2", BSC_INPUT, BSC_BINARY, NULL, NULL);
        bscAddEndpoint(&ep, "input", "3", BSC_INPUT, BSC_BINARY, NULL, NULL);
	bscAddEndpoint(&ep, "relay", "1", BSC_OUTPUT, BSC_BINARY, &cmdRelay, NULL);
	bscAddEndpoint(&ep, "relay", "2", BSC_OUTPUT, BSC_BINARY, &cmdRelay, NULL);
	bscAddEndpoint(&ep, "relay", "3", BSC_OUTPUT, BSC_BINARY, &cmdRelay, NULL);
	bscAddEndpoint(&ep, "relay", "4", BSC_OUTPUT, BSC_BINARY, &cmdRelay, NULL);
	bscAddEndpoint(&ep, "lcd",  NULL, BSC_OUTPUT, BSC_STREAM, &cmdLCD, NULL);

        bscAddEndpointFilterList(ep, INFO_INTERVAL);

	xapAddTimeoutAction(&timeout, 20, "user data");
	
	xapProcess();
	return 0;  // not reached
}
