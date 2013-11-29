/* $Id$
   Copyright (c) Brett England, 2010
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 
   The XIVELY adapter feeds data to the xively service on a regular
   basis.
 
   It's internal cache of data to push to XIVELY is updated by xAP
   messages being targetted at this service.
 
   Message sent to us are not immediately forwarded to the XIVELY
   service until the next update cycle.
 
   A Xively update message looks like this:
 
xap-header
{
v=12
hop=1
uid=FF00DA01
class=xively.update
target=dbzoo.livebox.xively
source=dbzoo.acme.test
}
datastream
{
id=3
tag=Outside Temperature
value=10.4
}
 
   A datastream will be created if it does not already exist.
*/

#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dscache.h"
#include "xap.h"
#include "mem.h"
#include "xively.h"

#define OPTIONAL 0
#define MANDATORY 1

char *interfaceName = "eth0";

char *inifile = "/etc/xap-livebox.ini";
static unsigned int g_ufreq;
char *g_apikey;
static unsigned long g_feedid;  // default feed

struct webFilter {
  // for XIVELY usage
  unsigned long feed;
  unsigned int id;
  char *min;
  char *max;
  char *unit;
  char *tag;
  // for xAP packet inspection
  char *section;
  char *key;
};

/** Receive an xap messages matching the webFilter [INI] conf.
* xapAddFilterAction callback
*/
void broadcastUpdate(void *userData)
{
	struct webFilter *wf = (struct webFilter *)userData;
	die_if(wf == NULL, "callback object NULL!");
        char *value = xapGetValue(wf->section, wf->key);
	float f;
	int now = 0;
	if(strcasecmp("on",value) == 0) { // Issue 27
		f = 1;
		now = 1; // A boolean state should never be cached.
	} else if(strcasecmp("off",value) == 0) {
		f = 0;
		now = 1;
	} else {
		f = atof(value);
	}
        updateDatastream(wf->feed, wf->id, wf->tag, f, wf->min, wf->max, wf->unit, now);
}

/** process an xAP xively update message.
* xapAddFilterAction callback
*/
void xivelyUpdate(void *userData)
{
	char *feed = xapGetValue("datastream","feed");
	char *id = xapGetValue("datastream","id");
        char *tag = xapGetValue("datastream","tag");
        char *value = xapGetValue("datastream","value");
        char *min = xapGetValue("datastream","min");
        char *max = xapGetValue("datastream","max");
        char *unit = xapGetValue("datastream","unit");
        int now = atoi(xapGetValue("datastream","now"));

        unsigned int idn = atoi(id);
        if(idn <= 0 && strcmp("0",id)) {
                warning("Invalid DATASTREAM section ID element: %s", id);
                return;
        }
	unsigned long feedn;
	if(feed) { // If supplied it must be valid.
	  feedn = atol(feed);
	  if(feedn <= 0) {
	    warning("Invalid DATASTREAM feed ID: %s", feed);
	    return;
	  }
	} else { // missing?  Default to global feed id.
	  feedn = g_feedid;
	}
        updateDatastream(feedn, idn, tag, atof(value), min, max, unit, now);
}


static char *getDynINI(char *key, unsigned int id, int mandatory)
{
        char dkey[32];
        char value[129];

        sprintf(dkey,"%s%d",key,id);
        long n = ini_gets("xively", dkey, "", value, sizeof(value), inifile);
        // Brutal death if all is not perfect.
	info("Loaded key %s=%s", dkey, value);
	if(n == 0) {
	  if(mandatory)
	    die("INI file missing [xively] %s=<value>", dkey);
	  return NULL;
	}
        return strdup(value);
}

/// Parse the INI file for additional configuration parameters.
void parseINI()
{
        char apikey[129];
        long feedid, n;

        ini_gets("xively","apikey","",apikey,sizeof(apikey),inifile);
        die_if(strlen(apikey) == 0, "apikey has not been setup\n");
	g_apikey = strdup(apikey);

        g_feedid = ini_getl("xively","feedid", 0, inifile);
        die_if(g_feedid == 0,"feedid has not been setup");

        g_ufreq = ini_getl("xively","ufreq",60,inifile);
	if(g_ufreq < 6 || g_ufreq > 900) {
                g_ufreq = 60; // default 60sec, 6 sec to 15 mins.
		warning("Update frequency out of range using default %d sec", g_ufreq);
	}

        long filters = ini_getl("xively","count",0,inifile);
        int i;
        char key[6];

        for(i=0; i < filters; i++) {
                // Construct an xAP filter based upon the WEB FILTER values
                struct webFilter *wf = (struct webFilter *)malloc(sizeof(struct webFilter));
	        char *id = getDynINI("id",i,MANDATORY);
                wf->id = atoi(id);
	        free(id);

	        id = getDynINI("feed",i,OPTIONAL);
		wf->feed = id ? atol(id) : g_feedid;
		free(id);

	        wf->min = getDynINI("min",i,OPTIONAL);
	        wf->max = getDynINI("max",i,OPTIONAL);
	        wf->unit = getDynINI("unit",i,OPTIONAL);

                wf->tag = getDynINI("tag",i,MANDATORY);
	        wf->section = getDynINI("section",i,MANDATORY);
                wf->key = getDynINI("key",i,MANDATORY);

                xAPFilter *xf = NULL;
                xapAddFilter(&xf, "xap-header", "source", getDynINI("source",i,MANDATORY));
	        xapAddFilter(&xf, "xap-header", "class", getDynINI("class",i,MANDATORY));
	        xapAddFilter(&xf, wf->section, wf->key, XAP_FILTER_ANY);
                xapAddFilterAction(&broadcastUpdate, xf, wf);
        }
}

int main(int argc, char *argv[])
{
        int i;
        printf("\nXively Connector for xAP v12\n");
        printf("Copyright (C) DBzoo 2009-2013\n\n");

	simpleCommandLine(argc, argv, &interfaceName);
        xapInitFromINI("xively","dbzoo.livebox","Xively","00D7",interfaceName,inifile);
        parseINI();

        // Setup XIVELY xAP service
        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", "xively.update");
        xapAddFilter(&f, "datastream", "id", XAP_FILTER_ANY);
        xapAddFilter(&f, "datastream", "tag", XAP_FILTER_ANY);
        xapAddFilter(&f, "datastream", "value", XAP_FILTER_ANY);
        xapAddFilterAction(&xivelyUpdate, f, NULL);

        // Update the XIVELY web service with our datastreams.
        xapAddTimeoutAction(&xivelyWebUpdate,  g_ufreq, NULL);

        xapProcess();
}
