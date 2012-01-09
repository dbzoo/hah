/* $Id$
   Copyright (c) Brett England, 2010
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 
   The PACHUBE adapter feeds data to the pachube service on a regular
   basis.
 
   It's internal cache of data to push to PACHUBE is updated by xAP
   messages being targetted at this service.
 
   Message sent to us are not immediately forwarded to the PACHUBE
   service until the next update cycle.
 
   A Pachube update message looks like this:
 
xap-header
{
v=12
hop=1
uid=FF00DA01
class=pachube.update
target=dbzoo.livebox.pachube
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
#include "pachube.h"

#define OPTIONAL 0
#define MANDATORY 1

char *interfaceName = "eth0";

char *inifile = "/etc/xap-livebox.ini";
static unsigned int g_ufreq;
char *g_apikey;
static unsigned int g_feedid;  // default feed

struct webFilter {
  // for PACHUBE usage
  unsigned int feed;
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
	if(strcasecmp("on",value) == 0) { // Issue 27
		f = 1;
	} else {
		f = atof(value);
	}
        updateDatastream(wf->feed, wf->id, wf->tag, f, wf->min, wf->max, wf->unit);
}

/** process an xAP pachube update message.
* xapAddFilterAction callback
*/
void pachubeUpdate(void *userData)
{
	char *feed = xapGetValue("datastream","feed");
	char *id = xapGetValue("datastream","id");
        char *tag = xapGetValue("datastream","tag");
        char *value = xapGetValue("datastream","value");
        char *min = xapGetValue("datastream","min");
        char *max = xapGetValue("datastream","max");
        char *unit = xapGetValue("datastream","unit");

        unsigned int idn = atoi(id);
        if(idn <= 0) {
                warning("Invalid DATASTREAM section ID element: %s", id);
                return;
        }
	unsigned int feedn;
	if(feed) { // If supplied it must be valid.
	  feedn = atoi(feed);
	  if(feedn <= 0) {
	    warning("Invalid DATASTREAM feed ID: %s", feed);
	    return;
	  }
	} else { // missing?  Default to global feed id.
	  feedn = g_feedid;
	}
        updateDatastream(feedn, idn, tag, atof(value), min, max, unit);
}


static char *getDynINI(char *key, unsigned int id, int mandatory)
{
        char dkey[32];
        char value[129];

        sprintf(dkey,"%s%d",key,id);
        long n = ini_gets("pachube", dkey, "", value, sizeof(value), inifile);
        // Brutal death if all is not perfect.
	info("Loaded key %s=%s", dkey, value);
	if(n == 0) {
	  if(mandatory)
	    die("INI file missing [pachube] %s=<value>", dkey);
	  return NULL;
	}
        return strdup(value);
}

/// Parse the INI file for additional configuration parameters.
void parseINI()
{
        char apikey[129];
        long feedid, n;

        ini_gets("pachube","apikey","",apikey,sizeof(apikey),inifile);
        die_if(strlen(apikey) == 0, "apikey has not been setup\n");
	g_apikey = strdup(apikey);

        g_feedid = ini_getl("pachube","feedid", 0, inifile);
        die_if(g_feedid == 0,"feedid has not been setup");

        g_ufreq = ini_getl("pachube","ufreq",60,inifile);
	if(g_ufreq < 6 || g_ufreq > 900) {
                g_ufreq = 60; // default 60sec, 6 sec to 15 mins.
		warning("Update frequency out of range using default %d sec", g_ufreq);
	}

        long filters = ini_getl("pachube","count",0,inifile);
        int i;
        char key[6];

        for(i=0; i < filters; i++) {
                // Construct an xAP filter based upon the WEB FILTER values
                struct webFilter *wf = (struct webFilter *)malloc(sizeof(struct webFilter));
	        char *id = getDynINI("id",i,MANDATORY);
                wf->id = atoi(id);
	        free(id);

	        id = getDynINI("feed",i,OPTIONAL);
		wf->feed = id ? atoi(id) : g_feedid;
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
        printf("\nPachube Connector for xAP v12\n");
        printf("Copyright (C) DBzoo 2009-2010\n\n");

	simpleCommandLine(argc, argv, &interfaceName);
        xapInitFromINI("pachube","dbzoo.livebox","Pachube","00D7",interfaceName,inifile);
        parseINI();

        // Setup PACHUBE xAP service
        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", "pachube.update");
        xapAddFilter(&f, "datastream", "id", XAP_FILTER_ANY);
        xapAddFilter(&f, "datastream", "tag", XAP_FILTER_ANY);
        xapAddFilter(&f, "datastream", "value", XAP_FILTER_ANY);
        xapAddFilterAction(&pachubeUpdate, f, NULL);

        // Update the PACHUBE web service with our datastreams.
        xapAddTimeoutAction(&pachubeWebUpdate,  g_ufreq, NULL);

        xapProcess();
}
