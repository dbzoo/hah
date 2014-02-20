/* $Id$
**
** Cache of datastreams
 */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "mem.h"
#include "dscache.h"
#include "log.h"

extern char *g_apikey;
static struct datastream *ds;
static int dscnt = 0;

// Unique list of Feeds to reduce HTTP rounds trips.
static unsigned long *feeds;
static unsigned int feeds_cnt = 0;

int appendXml(char **s, int *size, int offset, const char *fmt, ...) {
  int n = -1;
  va_list ap;
  char *p;

  info("Offset: %d Size: %d Space: %d", offset, *size, *size-offset);
  info("Process: %s", fmt);
  while(1) {
    p = *s + offset;
    // Try to print in the allocated space
    va_start(ap, fmt);
    n = vsnprintf(p, *size - offset, fmt, ap);
    va_end(ap);
    // if that worked, return
    if (n > -1 && n < *size - offset)
      return n;
    if (n > -1) //glibc 2.1
      *size += n+1;  // add precisely what is needed.
    else // glibc 2.0
      *size *= 2;  // twice the old
    info("Growing xml buffer to %d bytes", *size);
    *s = mem_realloc(*s, *size, M_NONE);
  }
}

/* Iterate over the datastream constructing an XML fragment in a
** static buffer suitable for pushing to xively as a data feed.
** Expand the buffer if necessary.
*/
static char *xmlDatastream(unsigned long feed) {
     static char *xml = NULL;
     static int xmllen = 200;
     int i, j, len;
     if(dscnt == 0) return NULL;

     if(xml == NULL) xml = mem_malloc(xmllen, M_NONE);
     j = 0;
     for(i=0; i < dscnt; i++) {
       if(ds[i].feed != feed) continue;

       // Issue 26 - round to 2 decimal points of value data.
       j += appendXml(&xml, &xmllen, j, "<data id=\"%d\"><tag>%s</tag><value", ds[i].id, ds[i].tag);

       // using V1 API.... v2 API would be <min_value>%s</min_value> etc..
       if(ds[i].min)
	 j += appendXml(&xml, &xmllen, j, " minValue=\"%s\"", ds[i].min);
       if(ds[i].max)
	 j += appendXml(&xml, &xmllen, j, " maxValue=\"%s\"", ds[i].max);       
       j += appendXml(&xml, &xmllen, j, ">%.2f</value>", ds[i].value);

       if(ds[i].unit)
	 j += appendXml(&xml, &xmllen, j, "<unit symbol=\"%s\">%s</unit>", ds[i].unit, ds[i].unit);

       j += appendXml(&xml, &xmllen, j, "</data>");
     }
     return xml;
}

/** Update the XIVELY webservice with our datastreams.
* xapAddTimeoutAction callback
*/
void xivelyWebUpdate(int interval, void *userData)
{
	// for each unique feed
	int i;
	for(i=0; i<feeds_cnt; i++) {
	  pach_updateDatastreamXml(feeds[i], g_apikey, xmlDatastream(feeds[i]));
	}
}


static int findDatastream(unsigned long feed, unsigned int id) {
     int i;
     for(i=0; i < dscnt; i++) {
          if(ds[i].id == id && ds[i].feed == feed) return i;
     }
     return -1;
}

// Add to the unique list of feeds being managed
static void addFeed(unsigned long feed) {
	int i;
	for(i=0; i<feeds_cnt; i++) {
		if(feeds[i] == feed) return;
	}
	info("Adding Feed %lu", feed);
	feeds_cnt++;
	feeds = mem_realloc(feeds, sizeof(long)*feeds_cnt, M_NONE);
	feeds[feeds_cnt-1] = feed;
}

void updateDatastream(unsigned long feed, unsigned int id, char *tag, float value, char *min, char *max, char *unit, int now) {
  int idx = findDatastream(feed, id);
     if(idx == -1) {
          info("NEW datastream");
	  dscnt++;
	  ds = mem_realloc(ds, sizeof(struct datastream) * dscnt, M_NONE);
	  idx = dscnt-1;

	  ds[idx].feed = feed;
	  ds[idx].id = id;
	  ds[idx].tag = mem_strdup(tag, M_NONE);
	  ds[idx].min = min ? mem_strdup(min, M_NONE) : NULL;
	  ds[idx].max = max ? mem_strdup(max, M_NONE) : NULL;
	  ds[idx].unit = unit ? mem_strdup(unit, M_NONE) : NULL;
	  addFeed(feed);
     }
     info("feed=%lu, id=%d, tag=%s, value=%.2f", ds[idx].feed, ds[idx].id, ds[idx].tag, value);
     ds[idx].value = value;

     if(now) {  // Expire the timer forcing it to be called and reset.
       xapTimeoutExpire(xapFindTimeoutByFunc(&xivelyWebUpdate));
     }
}
