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
const char *inifile = "/etc/xap-livebox.ini";

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
		len += snprintf(&newMsg[len], XAP_DATA_LEN-len,"target=%s\n", target);
	}
	strlcat(&newMsg[len], "}\n", XAP_DATA_LEN-len);

	parsedMsgToRawWithoutSection(xapBody, sizeof(xapBody),"xap-header");
	strlcat(newMsg, xapBody, XAP_DATA_LEN-len);

	xapSend(newMsg);
}

/// Display usage information and exit.
static void usage(char *prog)
{
	printf("%s: [options]\n",prog);
	printf("  -i, --interface IF     Default %s\n", interfaceName);
	printf("  -d, --debug            0-7\n");
	printf("  -h, --help\n\n");
	exit(1);
}

int main(int argc, char* argv[]) {
	int i;
	printf("\nxAP echo - for xAP v1.2\n");
	printf("Copyright (C) DBzoo 2012\n\n");

	xAPFilter *filter = NULL;

	for(i=0; i<argc; i++) {
		if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
			interfaceName = argv[++i];
		} else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
			setLoglevel(atoi(argv[++i]));
		} else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
			usage(argv[0]);
		}
	}

	xapInitFromINI("echo","dbzoo.livebox","Echo","00DF",interfaceName,inifile);

	xapAddFilterAction(&echoPacket, filter, NULL);
	xapProcess();
	return 0;
}


