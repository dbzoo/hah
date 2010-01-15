/* $Id$
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
#include <string.h>
#include <curl/curl.h>
#include "xapdef.h"
#include "xapGlobals.h"

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

const char inifile[] = "/etc/xap-livebox.ini";

char feedid[8];
char apikey[65];
int g_feed;
int g_ufreq;

char *eeml_head = "<eeml xmlns=\"http://www.eeml.org/xsd/005\""
	 "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
	 "xsi:schemaLocation=\"http://www.eeml.org/xsd/005 http://www.eeml.org/xsd/005/005.xsd\" version=\"5\">"
	 "<environment>";
char *eeml_foot ="</environment></eeml>";

#define XAP_MAX_KEYFIELD_LEN 200 // source:instance:type:section:keyvalue

struct datastream {
	 char id[4];
	 char xap[65];
	 char tag[20];
	 char value[XAP_MAX_KEYVALUE_LEN+1]; // XAP - last value
};
int filter_count;

struct datastream **ds;

/* Build HTTP Header.
   The only special we include is our Pachube API key.
 */
struct curl_slist *build_header() {
	 struct curl_slist *chunk = NULL;
	 char buff[256];
	 strcpy(buff,"X-PachubeApiKey: ");
	 strcat(buff, apikey);

	 chunk = curl_slist_append(chunk, buff);
	 chunk = curl_slist_append(chunk, "Expect:"); // disable this...
	 return chunk;
}

/* Extract the FEED ID from a URL of the form http://www.pachube.com/api/nnnn.xml
   return: nnnn
          -1 if no feed number found in the URL
*/
int getFeedID(char *url) {
	 char *lastslash = strrchr(url,'/');
	 char *lastdot = strrchr(url,'.');

	 char id[10];
	 int i = 0;
	 if(lastslash && lastdot) {
		  lastslash++;
		  while(*lastslash && lastslash < lastdot) {
			   id[i++] = *lastslash++;
			   if(i > sizeof(id))
					return -1;
		  }
		  id[i] = 0;
		  return atoi(id);
	 }
	 return -1;
}

int updateFeed() {
	 int i, j, update, len;
	 char eeml[2048];

	 update = 0;
	 strcpy(eeml, eeml_head);
	 for(i=0; i < filter_count; i++) {
		  if (g_debuglevel>5) printf("Filter: *%s*\n", ds[i]->xap);
		  j = strlen(eeml);
		  // if we have something in the cache
		  if(*ds[i]->value) {
			   len = snprintf(&eeml[j], sizeof(eeml),"<data id=\"%s\"><tag>%s</tag><value>%s</value></data>", 
							  ds[i]->id, ds[i]->tag, ds[i]->value);
			   if(len > sizeof(eeml)) { // buffer overflow - corrupt XML message!
					if (g_debuglevel) printf("*** updateFeed(): Buffer overflow ***\n");
					return;
			   }
			   update=1;
		  }
	 }
	 strcat(eeml, eeml_foot);
	 if(update == 0) return; // No datastream in the cache yet.

	 if (g_debuglevel>5) printf("Update FEED: %s\n", eeml);

	 char url[100];
	 strcpy(url,"http://www.pachube.com/api/");
	 strcat(url, feedid);
	 strcat(url,".xml?_method=put");

	 CURL *curl;
	 CURLcode res;
	 long code;
	 curl = curl_easy_init();
	 if(curl) {
		  struct curl_slist *chunk = build_header();

		  char buff[64];
		  sprintf(buff,"Content-Length: %d", strlen(eeml));
		  chunk = curl_slist_append(chunk, buff);

		  curl_easy_setopt(curl, CURLOPT_URL, url);
		  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, eeml);
		  curl_easy_setopt(curl, CURLOPT_POST, 1L);
		  if(g_debuglevel >= 7) 
			   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		  res = curl_easy_perform(curl);
		  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		  curl_easy_cleanup(curl);

		  if(code != 200) {
			   code = -code;
		  }
		  return code;
	 }
	 return -1;
}

int filterHasKey(char *key, int *idx) { 
	 int matched = 0;
	 int i = 0;
	 if(g_debuglevel >=6) printf("Filter Search %s\n",key);
	 while(!matched && (i < filter_count)) {
		  if(strcasecmp(ds[i]->xap, key) == 0) {
			   matched = 1;
			   *idx=i;
		  }
		  i++;
	 }
	 if(g_debuglevel >= 6) printf("Result: %s\n", matched?"HIT":"MISS", key);
	 return matched;
}

void filterUpdateIdx(int idx, char *value) {
	 if (g_debuglevel>4) printf("Filter: Updating %s=%s\n",ds[idx]->xap,value);
	 strcpy(ds[idx]->value, value);
}

// Receive a xap (broadcast) packet and process
void xap_handler(const char* a_buf) {
	 char i_source[XAP_MAX_KEYVALUE_LEN];
	 char i_class[XAP_MAX_KEYVALUE_LEN];
	 int i;

	 xapmsg_parse(a_buf);

	 if(xapmsg_gettype() != XAP_MSG_ORDINARY) return;

	 if (xapmsg_getvalue("xap-header:source", i_source)==0) {
		 if (g_debuglevel>3) printf("No source in header\n");
		 return;
	 }
			
	 if (xapmsg_getvalue("xap-header:class", i_class)==0) {
		  if (g_debuglevel>3) printf("No class in header\n");
		  return;
	 }

	 // Pull sections and key/value pairs for this message
	 char xap_key[XAP_MAX_KEYFIELD_LEN];
	 for(i=0; i<g_xap_index; i++) {
		  strcpy(xap_key, i_source);
		  strcat(xap_key, ":");
		  strcat(xap_key, i_class);
		  strcat(xap_key, ":");
		  strcat(xap_key, g_xap_msg[i].name);

		  int idx;
		  if(filterHasKey(xap_key, &idx)) {
			   filterUpdateIdx(idx, g_xap_msg[i].value);
		  }
	 }
}

void process_event() {
	 struct timeval i_tv;
	 fd_set master, i_rdfs;
	 int i_retval;
	 char i_xap_buff[1500+1];   // incoming XAP message buffer
	 int i;

	 // Setup the select() sets.
	 FD_ZERO(&master);
	 FD_SET(g_xap_receiver_sockfd, &master);
	 
	 while (1)
	 {
		  i_tv.tv_sec=HBEAT_INTERVAL;
		  i_tv.tv_usec=0;
	 
		  i_rdfs = master; // copy it - select() alters this each call.
		  i_retval = select(g_xap_receiver_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);

		  // if incoming XAP message...
		  // Check for new incoming XAP messages.  They are handled in xap_handler.
		  if (FD_ISSET(g_xap_receiver_sockfd, &i_rdfs)) {
			   if (xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buff, sizeof(i_xap_buff))>0) {
					xap_handler(i_xap_buff);
			   }
		  }

		  // Update Pachube every TICK
		  if (xap_send_tick(g_ufreq)) {
			   updateFeed();
		  }

		  // Send heartbeat periodically
		  xap_heartbeat_tick(HBEAT_INTERVAL);
	 }
}

void loadDynINI(char *key, int id, char *location, int size) {
	 char buff[100];
	 sprintf(buff,"%s%d",key,id);
	 ini_gets("pachube", buff, "", location, size, inifile);
}

void setupXAPini() {
	 char uid[5];
	 char guid[9];
	 long n;

	 // default to 00DE if not present
	 n = ini_gets("pachube","uid","00DE",uid,sizeof(uid),inifile);

	 // validate that the UID can be read as HEX
	 if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
					isxdigit(uid[2]) && isxdigit(uid[3]))) 
	 {
		  strcpy(uid,"00DE");
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

	 ini_gets("pachube","apikey","",apikey,sizeof(apikey),inifile);

	 filter_count = ini_getl("pachube","count",0,inifile);
	 int i, ds_count = 0;
	 ds = (struct datastream **)malloc(filter_count * sizeof(struct datastream *));
	 struct datastream temp_ds;
	 for(i=0;i<filter_count;i++) {
		  memset(&temp_ds, 0, sizeof(struct datastream));
		  loadDynINI("id", i, temp_ds.id, sizeof(temp_ds.id) );
		  loadDynINI("xap", i, temp_ds.xap, sizeof(temp_ds.xap) );
		  loadDynINI("tag", i, temp_ds.tag, sizeof(temp_ds.tag) );

		  // Validate the .ini file data (at least check its not empty)
		  if(*temp_ds.tag && *temp_ds.id && *temp_ds.xap) {
			   ds[i] = (struct datastream *)malloc(sizeof(struct datastream));
			   memcpy(ds[i], &temp_ds, sizeof(temp_ds));
			   ds_count++;
		  }
	 }
	 filter_count = ds_count; // Adjust for those we really added.

	 ini_gets("pachube","feedid", "", feedid, sizeof(feedid), inifile);

	 g_ufreq = ini_getl("pachube","ufreq",60,inifile);
	 if(g_ufreq < 6 || g_ufreq > 900) g_ufreq = 60; // default 60sec, 6 sec to 15 mins.
}

int main(int argc, char *argv[]) {
	 printf("\nPachube Connector for xAP v12\n");
	 printf("Copyright (C) DBzoo 2009\n\n");

	 setupXAPini();

	 if(strlen(apikey) == 0) {
		  printf("Error: apikey has not been setup\n");
		  exit(1);
	 }
	 if(strlen(feedid) == 0) {
		  printf("Error: feedid has not been setup\n");
		  exit(1);
	 }
	 if(filter_count == 0) {
		  printf("Error: datastreams have not been setup\n");
		  exit(1);
	 }

	 xap_init(argc, argv, 0);
	 process_event();
}
