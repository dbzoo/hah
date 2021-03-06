/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xap.h"

xAP *gXAP;

int main(int argc, char **argv) {
	setLoglevel(7);
	
	xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.1.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.1.>");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.*.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.>");
	xapAddFilter(&f, "xap-header","target",XAP_FILTER_ANY);
	xapAddFilter(&f, "xap-header","target",XAP_FILTER_ABSENT);
	
	xAPFilter *e;
	for(e = f; e; e = e->next) {
		int match = xapFilterAddrSubaddress(e->value, "dbzoo.livebox.1.controller");
		printf("%s { %s=%s } - %d\n", e->section, e->key, e->value, match);
	}
	return 0;
}
