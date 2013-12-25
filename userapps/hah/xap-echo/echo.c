/* $Id$
   xAP echo - capture every incoming packet rewraps the header and spits it out.
   Perhaps useful for debugging and stress testing.

   Copyright (c) Brett England, 2012
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xap.h"

char *interfaceName = "br0";
const char *inifile = "/etc/xap.d/xap-echo.ini";

char newMsg[XAP_DATA_LEN];
char xapBody[XAP_DATA_LEN];

// Take an incoming packet strip its header and xmit the section as an echo packet.
void echoPacket(void *userData) {
	if(xapGetType() != XAP_MSG_ORDINARY) return;
	if (strcmp(xapGetValue("xap-header","source"),"dbzoo.livebox.test.Echo") == 0) return;

	int len = snprintf(newMsg, XAP_DATA_LEN, "xap-header\n"
			   "{\n"
			   "v=12\n"
			   "hop=1\n"
			   "uid=%s\n"
			   "class=echo\n"
			   "source=%s\n" 
			   "}\n"
			   "old-header\n"
			   "{\n"
			   "uid=%s\n"
			   "source=%s\n"
			   "class=%s\n"
			   , xapGetUID(), xapGetSource(), 
			   xapGetValue("xap-header","uid"),
			   xapGetValue("xap-header","source"), 
			   xapGetValue("xap-header","class"));
	char *target = xapGetValue("xap-header","target");
	if(target) {
		snprintf(&newMsg[len], XAP_DATA_LEN-len,"target=%s\n", target);
	}
	strlcat(newMsg, "}\n", XAP_DATA_LEN);

	parsedMsgToRawWithoutSection(xapBody, sizeof(xapBody),"xap-header");
	strlcat(newMsg, xapBody, XAP_DATA_LEN);

	xapSend(newMsg);
}

int main(int argc, char* argv[]) {
	printf("\nxAP echo - for xAP v1.2\n");
	printf("Copyright (C) DBzoo 2012\n\n");

	xAPFilter *filter = NULL;

	simpleCommandLine(argc, argv, &interfaceName);
	xapInitFromINI("echo","dbzoo","Echo","00DF",interfaceName,inifile);

	xapAddFilterAction(&echoPacket, filter, NULL);
	xapProcess();
	return 0;
}


