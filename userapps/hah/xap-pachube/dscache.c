/* $Id$
**
** Cache of datastreams
 */
#include <string.h>
#include <stdio.h>
#include "mem.h"
#include "dscache.h"
#include "debug.h"

static struct datastream *ds;
static int dscnt;

/* Iterate over the datastream constructing an XML fragment in a
** static buffer suitable for pushing to pachube as a data feed.
** Expand the buffer if necessary.
*/
char *xmlDatastream() {
     static char *xml = NULL;
     static int xmllen = 512;
     int i, j, len;
     if(dscnt == 0) return NULL;

     if(xml == NULL) xml = mem_malloc(xmllen, M_NONE);
again:
     j = 0;
     for(i=0; i < dscnt; i++) {
	  j += snprintf(&xml[j], xmllen-j,"<data id=\"%d\"><tag>%s</tag><value>%f</value></data>", 
			 ds[i].id, ds[i].tag, ds[i].value);
	  if(j >= xmllen) { // out of buffer?
	       xmllen += 512;
	       xml = mem_realloc(xml, xmllen, M_NONE);
	       goto again;
	  }
     }
     return xml;
}

static int findDatastream(int id) {
     int i;
     for(i=0; i < dscnt; i++) {
          if(ds[i].id == id) return i;
     }
     return -1;
}

void updateDatastream(int id, char *tag, float value) {
     int idx = findDatastream(id);
     if(idx == -1) {
	  dscnt++;
	  ds = mem_realloc(ds, sizeof(struct datastream) * dscnt, M_NONE);
	  idx = dscnt-1;
     }
     debug(LOG_DEBUG,"updateDatastream(): id=%d, tag=%s, value=%f\n", id, tag, value);
     ds[idx].id = id;
     strcpy(ds[idx].tag, tag);
     ds[idx].value = value;
}

#ifdef TEST
int g_debuglevel = 9;

void dumpDS() {
     int i;
     for(i=0; i < dscnt; i++) {
          printf("%d: id=%d, tag=%s, value=%f\n", i, ds[i].id, ds[i].tag, ds[i].value);
     }
}

main(int argc, char **argv) {
     updateDatastream(1, "hello", 3);
     updateDatastream(2, "there", 5);
     updateDatastream(3, "world", 7);
     dumpDS();
     printf("%s\n",xmlDatastream());
}
#endif
