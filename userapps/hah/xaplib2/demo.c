/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
// Minimal shell for an xAP compliance
#include <stdio.h>
#include <stdlib.h>
#include "xap.h"
#include "bsc.h"

#define INFO_INTERVAL 120

void cmd (xAP *xap, bscEndpoint *e) {
	printf("XAP Command received for %s.%s\n", e->name, e->subaddr);
}

void infoEvent1wire (xAP *xap, bscEndpoint *e, char *clazz) {
	// Lazy memory allocation of displayText
	if(e->displayText == NULL) e->displayText = (char *)malloc(20);
	snprintf(e->displayText, 20, "Temperature %s C", e->text);
	
	bscInfoEvent(xap, e, clazz);
}

int main(int argc, char *argv[]) {
	xAP *x = xapNew("dbzoo.livebox.Demo","FF00DB00", "eth0");
	
	bscEndpoint *ep = NULL;	
	bscAddEndpoint(&ep, "1wire", "1", "01", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
	bscAddEndpoint(&ep, "1wire", "2", "02", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
	bscAddEndpoint(&ep, "input", "1", "03", BSC_INPUT, BSC_BINARY, NULL, NULL);
	bscAddEndpoint(&ep, "input", "2", "04", BSC_INPUT, BSC_BINARY, NULL, NULL);
	bscAddEndpoint(&ep, "input", "3", "05", BSC_INPUT, BSC_BINARY, NULL, NULL);	
	bscAddEndpoint(&ep, "relay", "1", "06", BSC_OUTPUT, BSC_BINARY, &cmd, NULL);
	bscAddEndpoint(&ep, "relay", "2", "07", BSC_OUTPUT, BSC_BINARY, &cmd, NULL);
	bscAddEndpoint(&ep, "relay", "3", "08", BSC_OUTPUT, BSC_BINARY, &cmd, NULL);
	bscAddEndpoint(&ep, "relay", "4", "09", BSC_OUTPUT, BSC_BINARY, &cmd, NULL);
	bscAddEndpoint(&ep, "lcd",  NULL, "10", BSC_OUTPUT, BSC_STREAM, &cmd, NULL);
	
	xapAddBscEndpointFilters(x, ep, INFO_INTERVAL);

	xapProcess(x);
	return 0;
}
