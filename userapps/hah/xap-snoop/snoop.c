/* $Id$
   xAP snoop

  Copyright (c) Brett England, 2009
   
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

char *interfaceName = NULL;
const char *inifile = "/etc/xap.d/xap-snoop.ini";

void packetDump(void *userData) {
	char newmsg[XAP_DATA_LEN];
	parsedMsgToRaw(newmsg, sizeof(newmsg));
	printf("%s\n", newmsg);
}

/// Display usage information and exit.
static void usage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF\n");
        printf("  -d, --debug            0-7\n");
        printf("  -h, --help\n\n");
	printf("Each of these may appear multiple times\n");
        printf("  -s, --source <value>\n");
        printf("  -t, --target <value>\n");
        printf("  -c, --class  <value>\n");
        printf("  -f, --filter <section> <key> <value>\n\n");
        printf("XAP pattern matching applies\n");
        printf("Examples:\n");
        printf("-t dbzoo.livebox.GoogleCal\n");
        printf("-s dbzoo.livebox.Controller:>\n");
        exit(1);
}

int main(int argc, char* argv[]) {
	int i;
	printf("\nxAP Snoop - for xAP v1.2\n");
	printf("Copyright (C) DBzoo 2009-2010\n\n");

	xAPFilter *filter = NULL;

        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        interfaceName = argv[++i];
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
                        usage(argv[0]);
                } else if(strcmp("-s", argv[i]) == 0 || strcmp("--source", argv[i]) == 0) {
                        xapAddFilter(&filter, "xap-header", "source", argv[++i]);
                } else if(strcmp("-t", argv[i]) == 0 || strcmp("--target", argv[i]) == 0) {
                        xapAddFilter(&filter, "xap-header", "target", argv[++i]);
                } else if(strcmp("-c", argv[i]) == 0 || strcmp("--class", argv[i]) == 0) {
                        xapAddFilter(&filter, "xap-header", "class", argv[++i]);
                } else if(strcmp("-f", argv[i]) == 0 || strcmp("--filter", argv[i]) == 0) {
			char *section = argv[++i];
			char *key = argv[++i];
			char *value = argv[++i];
                        xapAddFilter(&filter, section, key, value);
                }
        }

	xapInitFromINI("snoop","dbzoo","Snoop","00D4",interfaceName,inifile);

	xapAddFilterAction(&packetDump, filter, NULL);
	xapProcess();
	return 0;
}
