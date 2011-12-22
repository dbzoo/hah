/* $Id$
**
** Cache of datastreams
 */
#include <string.h>
#include <stdio.h>
#include "mem.h"
#include "dscache.h"
#include "log.h"

extern char *g_apikey;
static struct datastream *ds;
static int dscnt = 0;

// Unique list of Feeds to reduce HTTP rounds trips.
static unsigned int *feeds;
static unsigned int feeds_cnt = 0;

/* Iterate over the datastream constructing an XML fragment in a
** static buffer suitable for pushing to pachube as a data feed.
** Expand the buffer if necessary.
*/
static char *xmlDatastream(unsigned int feed) {
     static char *xml = NULL;
     static int xmllen = 512;
     int i, j, len;
     if(dscnt == 0) return NULL;

     if(xml == NULL) xml = mem_malloc(xmllen, M_NONE);
again:
     j = 0;
     for(i=0; i < dscnt; i++) {
	  if(ds[i].feed != feed) continue;
	     // Issue 26 - round to 2 decimal points of value data.
	  j += snprintf(&xml[j], xmllen-j,"<data id=\"%d\"><tag>%s</tag><value>%.2f</value></data>", 
			 ds[i].id, ds[i].tag, ds[i].value);
	  if(j >= xmllen) { // out of buffer?
	       xmllen += 512;
	       xml = mem_realloc(xml, xmllen, M_NONE);
	       goto again;
	  }
     }
     return xml;
}

/** Update the PACHUBE webservice with our datastreams.
* xapAddTimeoutAction callback
*/
void pachubeWebUpdate(int interval, void *userData)
{
	// for each unique feed
	int i;
	for(i=0; i<feeds_cnt; i++) {
		pach_updateDatastreamXml(feeds[i], g_apikey, xmlDatastream(feeds[i]));
	}
}


static int findDatastream(unsigned int feed, unsigned int id) {
     int i;
     for(i=0; i < dscnt; i++) {
          if(ds[i].id == id && ds[i].feed == feed) return i;
     }
     return -1;
}

// Add to the unique list of feeds being managed
static void addFeed(unsigned int feed) {
	int i;
	for(i=0; i<feeds_cnt; i++) {
		if(feeds[i] == feed) return;
	}
	info("Adding Feed %d", feed);
	feeds_cnt++;
	feeds = mem_realloc(feeds, sizeof(int)*feeds_cnt, M_NONE);
	feeds[feeds_cnt-1] = feed;
}

void updateDatastream(unsigned int feed, unsigned int id, char *tag, float value) {
  int idx = findDatastream(feed, id);
     if(idx == -1) {
	  dscnt++;
	  ds = mem_realloc(ds, sizeof(struct datastream) * dscnt, M_NONE);
	  idx = dscnt-1;
     }
     info("feed=%d, id=%d, tag=%s, value=%f", feed, id, tag, value);
     ds[idx].feed = feed;
     ds[idx].id = id;
     strcpy(ds[idx].tag, tag);
     ds[idx].value = value;
     addFeed(feed);
}
