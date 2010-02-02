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
#include <string.h>
#include <curl/curl.h>
#include "dscache.h"
#include "debug.h"
#include "mem.h"
#include "pachube.h"
#include "xapdef.h"
#include "xapGlobals.h"

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

const char *inifile = "/etc/xap-livebox.ini";
static int g_ufreq;
static pach_t g_pachube;

#define min(x,y) (x)<(y)?(x):(y)
#define XAP_MAX_KEYFIELD_LEN 200 // source:instance:type:section:keyvalue

// Filters are WEB GUI supplied easy filters as parsed from the .ini file.
static struct _filter {
     int id;
     char tag[DS_TAGSIZE];
     char xapFilter[XAP_MAX_KEYFIELD_LEN];
} *filter = NULL;
static int filtercnt=0;

void addFilter(struct _filter *f) {
     debug(LOG_INFO,"addFilter(): %d %s %s", f->id, f->tag, f->xapFilter);
     filtercnt++;
     filter = mem_realloc(filter, sizeof(struct _filter) * filtercnt, M_NONE);
     memcpy(&filter[filtercnt-1], f, sizeof(struct _filter));
}

int updateFilter(char *xapFilter, char *value) { 
     int i;
     for(i=0; i < filtercnt; i++) {
	  struct _filter *f = &filter[i];
	  if(strcasecmp(f->xapFilter, xapFilter) == 0) {
	       updateDatastream(f->id, f->tag, atof(value));
	       return 0;
	  }
     }
     return -1;
}

// Receive a xap (broadcast) packet and process
void xap_handler(const char* a_buf) 
{
     char i_target[XAP_MAX_KEYVALUE_LEN];
     char i_identity[XAP_MAX_KEYVALUE_LEN];
     char i_source[XAP_MAX_KEYVALUE_LEN];
     char i_class[XAP_MAX_KEYVALUE_LEN];

     xapmsg_parse(a_buf);

     if(xapmsg_gettype() != XAP_MSG_ORDINARY) return;
     // Every message must have a class and source to a candidate.
     if (xapmsg_getvalue("xap-header:source", i_source) == 0 ||
	 xapmsg_getvalue("xap-header:class", i_class) == 0 )
     {
	  return;
     }

     // If a NON-TARGETTED message ie, event/info class type.
     if (xapmsg_getvalue("xap-header:target", i_target) == 0) {
	  // Pull sections and key/value pairs for this message
	  // and update and WEB GUI easy filters.
	  char xap_key[XAP_MAX_KEYFIELD_LEN];
	  int i;
	  for(i=0; i<g_xap_index; i++) {
	       strlcpy(xap_key, i_source, sizeof(xap_key));
	       strlcat(xap_key, ":", sizeof(xap_key));
	       strlcat(xap_key, i_class, sizeof(xap_key));
	       strlcat(xap_key, ":", sizeof(xap_key));
	       strlcat(xap_key, g_xap_msg[i].name, sizeof(xap_key));
	       updateFilter(xap_key, g_xap_msg[i].value);
	 }
     } else {
	  // Is it the right class type?
	  if(strcasecmp(i_class, "pachube.update")) return;
	  // was the 'target=' for this instance?
	  snprintf(i_identity, sizeof(i_identity), "%s.%s.%s", XAP_ME, XAP_SOURCE, g_instance);
	  if (xap_compare(i_target, i_identity) != 1 ) return;

	  // Process a directed PACHUBE update XAP message.
	  char id[4];
	  if (xapmsg_getvalue("datastream:id", id)==0) {
	       debug(LOG_WARNING,"Missing DATASTREAM section ID element");
	       return;
	  }
	  int idn = atoi(id);
	  if(idn == 0) {
	       debug(LOG_WARNING,"Invalid DATASTREAM section ID element: %s", id);
	       return;
	  }

	  char value[10];
	  if (xapmsg_getvalue("datastream:value", value)==0) {
	       debug(LOG_WARNING,"Missing DATASTREAM section VALUE element");
	       return;
	  }

	  char tag[DS_TAGSIZE];
	  if (xapmsg_getvalue("datastream:tag", tag)==0) {
	       debug(LOG_WARNING,"Missing DATASTREAM section TAG element");
	       return;
	  }

	  updateDatastream(idn, tag, atof(value));
     } 
}

void process_event() {
     struct timeval tv;
     fd_set rdfs;
     char xap_buff[1500];
     int ticker = min(HBEAT_INTERVAL,g_ufreq);

     debug(LOG_INFO,"Processing events...");
     for(;;)
     {
	  tv.tv_sec=ticker;
	  tv.tv_usec=0;
	 
	  FD_ZERO(&rdfs);
	  FD_SET(g_xap_receiver_sockfd, &rdfs);
	  select(g_xap_receiver_sockfd+1, &rdfs, NULL, NULL, &tv);

	  // if incoming XAP message...
	  // Check for new incoming XAP messages.  They are handled in xap_handler.
	  if (FD_ISSET(g_xap_receiver_sockfd, &rdfs)) {
	       if (xap_poll_incoming(g_xap_receiver_sockfd, xap_buff, sizeof(xap_buff))>0) {
		    xap_handler(xap_buff);
	       }
	  }

	  // Update Pachube every TICK
	  if (xap_send_tick(g_ufreq)) {
	       pach_updateDatastreamXml(g_pachube, xmlDatastream());
	  }

	  // Send heartbeat periodically
	  xap_heartbeat_tick(HBEAT_INTERVAL);
     }
}

// Setup XAP specific variables.
void setupXAPini() {
     char uid[5];
     char guid[9];
     long n;

     // default to 00D7 if not present
     n = ini_gets("pachube","uid","00D7",uid,sizeof(uid),inifile);

     // validate that the UID can be read as HEX
     if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
		    isxdigit(uid[2]) && isxdigit(uid[3]))) 
     {
	  strcpy(uid,"00D7");
     }
     snprintf(guid,sizeof guid,"FF%s00", uid);
     XAP_GUID = strdup(guid);

     char control[30];
     n = ini_gets("pachube","instance","Pachube",control,sizeof(control),inifile);
     if(n == 0 || strlen(control) == 0) 
     {
	  strcpy(control,"Pachube");
     }
     XAP_DEFAULT_INSTANCE = strdup(control);
}

void parseINI() {
     char apikey[100];
     long feedid;

     debug(LOG_INFO,"ParseINI %s",inifile);
     ini_gets("pachube","apikey","",apikey,sizeof(apikey),inifile);
     if(strlen(apikey) == 0) {
	  debug(LOG_EMERG,"Error: apikey has not been setup\n");
	  exit(1);
     }

     feedid = ini_getl("pachube","feedid", 0, inifile);
     if(feedid == 0) {
	  debug(LOG_EMERG,"Error: feedid has not been setup\n");
	  exit(1);
     }

     g_ufreq = ini_getl("pachube","ufreq",60,inifile);
     if(g_ufreq < 6 || g_ufreq > 900) g_ufreq = 60; // default 60sec, 6 sec to 15 mins.

     g_pachube = pach_new(apikey, feedid);

     long filters = ini_getl("pachube","count",0,inifile);
     int i;
     char buff[8];
     struct _filter f;
     for(i=0; i < filters; i++) {
	  sprintf(buff,"id%d",i);
	  f.id = ini_getl("pachube",buff,0,inifile);

	  sprintf(buff,"xap%d",i);
	  ini_gets("pachube", buff, "", f.xapFilter, sizeof(f.xapFilter), inifile);	  

	  sprintf(buff,"tag%d",i);
	  ini_gets("pachube", buff, "", f.tag, sizeof(f.tag), inifile);	  

	  if(strlen(f.tag) > 0 && strlen(f.xapFilter) > 0 && f.id >= 0)
	     addFilter(&f);
     }
}

int main(int argc, char *argv[]) {
     printf("\nPachube Connector for xAP v12\n");
     printf("Copyright (C) DBzoo 2009\n\n");

     setupXAPini();
     xap_init(argc, argv, 0);
     parseINI();
     process_event();
}
