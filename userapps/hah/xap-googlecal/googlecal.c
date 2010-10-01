/* $Id$
   Copyright (c) Brett England, 2009

   Google calendar xap daemon.

   This program will monitor a calendar and produce an xAP payload every minute
   for as long as the activity is running.

   An entry is setup in the calendar using the following fields:

   What: <Some text defining your event>
   Where: XAP   <must be the value xap>
   When: Start-time to Stop-time
   Description: An xap payload using short form notation.

   xap-header
   {
   class=xAPBSC.cmd
   target=dbzoo.livebox.controller:lcd
   }
   output.state.1
   {
   id=*
   text=Hello World
   }

   In the header you only have to specify the XAP message type and the target.
   The other fields will be automatically populated for you.
   
   The event will fire every minute for the duration of the start/stop time.
   If you require a single message then make the start/stop time the same value.

   It also allows others to schedule google calendar events by place an xap
   message on the bus.

   target=dbzoo.livebox.GoogleCal
   class=google.calendar
   event 
   {
   title=
   start=
   end=
   description=
   }

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
#include <time.h>
#include <gcalendar.h>
#include <internal_gcal.h>
#include "xapdef.h"
#include "xapGlobals.h"
#include "svn_date.h"

#define XAP_CLASS "google.calendar"
// Frequency in seconds that we check for a trigger event.
// As google calendar can only define to the minute this number
// reflects that granulality.
#define EVENT_FREQ 60

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

static int freq; // calendar sync frequency
static char username[64];
static char password[64];

const char inifile[] = "/etc/xap-livebox.ini";
//const char inifile[] = "sample.ini";

static gcal_t gcal;

// Debug helper: Display event
void dump_event(gcal_event_t event) 
{
     printf("event(title=%s,start=%s,end=%s,where=%s,content=%s)\n", 
	    gcal_event_get_title(event), //what
	    gcal_event_get_start(event),
	    gcal_event_get_end(event),
	    gcal_event_get_where(event),
	    gcal_event_get_content(event) //description
	  );
}

// Debug helper: Display event cache.
void dump_events(struct gcal_event_array *event_array) 
{
     gcal_event_t event;
     int i;

     printf("Dump event cache - found %d events\n", event_array->length);
     for(i=0; i<event_array->length; i++) {
	  event = gcal_event_element(event_array, i);
	  if(event) dump_event(event);
     }
}

// The internal function get_mili_timestamp can't take an offset.
// It only produces timestamp at NOW.  So its been cloned here and the offset
// parameter has been added.  This also aligns the times onto a minute boundary.
int get_offset_timestamp(char *timestamp, size_t length, int offset, char *atimezone)
{
     struct tm loctime;
     time_t curtime;
     struct timeval detail_time;
     char buffer[12];
	
     if (!timestamp || length < TIMESTAMP_SIZE)
	  return -1;

     curtime = time(NULL) + offset;
     localtime_r(&curtime, &loctime);
     loctime.tm_sec = 0; // on minute boundary

     strftime(timestamp, length - 1, "%FT%T", &loctime);
     //strcat(timestamp,".000");
     if (atimezone)
	  strncat(timestamp, atimezone, length);
     else
	  strncat(timestamp, "Z", length);

     return 0;
}

// useful to switch out get_period_events
int get_all_events(struct gcal_event_array *events) {
     return gcal_get_events(gcal, events);
}

// Get all events between now and the next polling period
int get_period_events(struct gcal_event_array *events) 
{
     char start_timestamp[TIMESTAMP_MAX_SIZE];
     char stop_timestamp[TIMESTAMP_MAX_SIZE];
     int result = -1;
     char query[256];

     if(events) events->length = 0;

     // to access gcal members we had to include <internal_gcal.h>  
     result = get_offset_timestamp(start_timestamp, TIMESTAMP_MAX_SIZE, 0, gcal->timezone);
     if(result) return result;
     result = get_offset_timestamp(stop_timestamp, TIMESTAMP_MAX_SIZE, freq, gcal->timezone);
     if(result) return result;

     // http://code.google.com/apis/calendar/data/2.0/reference.html#Parameters
     snprintf(query, sizeof(query), "start-min=%s&start-max=%s&singleevents=true", 
	      start_timestamp, stop_timestamp);
     if(g_debuglevel >= 6) {
	  printf("Google query params: %s\n", query);
     }
     result = gcal_query(gcal, query);
     if (result) {
	  if(g_debuglevel >= 6) {
	       printf("Google returns no events\n");
	  }
	  events->entries = NULL;
	  events->length = 0;
	  return result;
     }

     events->entries = gcal_get_entries(gcal, &events->length);
     if(events->entries) result = 0;
	
     if(g_debuglevel >= 6) {
	  dump_events(events);
     }

     return result;
}

/* Search the array of pending calendar events and if its active action it.
 */
void fire_google_calendar_event(struct gcal_event_array *event_array) 
{
     gcal_event_t event;
     int result;
     int i;
     time_t start_time = (time_t)NULL, stop_time = (time_t)NULL;
     time_t curtime = time(NULL);

     for(i=0; i<event_array->length; i++) {
	  event = gcal_event_element(event_array, i);
	  if(!event) break;

	  if(g_debuglevel >= 6) {
	       printf("Trigger ");
	       dump_event(event);
	  }

	  // Convert the event ISO8601 timestamp - we never expect these to fail...	
	  char *date = gcal_event_get_start(event);
	  if(svn_parse_date(date, curtime, &start_time) != 0) {
	       if(g_debuglevel) printf("Error parsing START date: %s\n", date);
	       continue;
	  }
	  
	  date = gcal_event_get_end(event);
	  if(svn_parse_date(date, curtime, &stop_time) != 0) {
	       if(g_debuglevel) printf("Error parsing END date: %s\n", date);
	       continue;
	  }

	  // Is the event ready to fire?
	  if(start_time > curtime && stop_time < curtime) {
	       if(g_debuglevel >= 6) printf("Event not ready to fire\n");
	       continue;
	  }

	  // Only calendar events with the WHERE field = xap are valid.
	  // We do this to make life easier for us known that only events
	  // marked a certain way are destined for us.  Prevents the users
	  // other events from inadvertantly firing us with bad payloads.
	  if(strcasecmp("xap",gcal_event_get_where(event))) {
	       if(g_debuglevel >= 7) printf("where<>xap : Not an XAP event\n");
	       continue;
	  }

	  // Check against the ALIAS patterns first.

	  char *description = gcal_event_get_content(event);
	  char *xapmsg = NULL;

	  // An XAP message but no description.
	  // This means its an alias as they won't have xAP payloads.
	  if(description && *description) {
	       // The "description" field from the calendar event
	       // contains the xAP fragment we want to send
	       // As this is a partial xAP message fix it up.
	       xapmsg = normalize_xap(description);
	  } else {
	       // pattern match the title against the aliases to find the xAP msg.
	       xapmsg = (char *)malloc(sizeof(char)*1500);
	       snprintf(xapmsg, sizeof(xapmsg),
			"xap-header\n{\nv=12\nhop=1\nuid=%s\n"
			"class=alias\nsource=%s.%s.%s\n}\n"
			"command\n{\ntext=%s\n}\n", 
			g_uid, XAP_ME, XAP_SOURCE, g_instance, gcal_event_get_title(event));
	       
	  }
	  if(xapmsg) {
	       if(g_debuglevel) printf("Send XAP message\n%s\n", xapmsg);
	       xap_send_message(xapmsg);
	       free(xapmsg);
	  }
     } 
}

void to_iso8601(char *date, int date_len, time_t *then) 
{
     struct tm tm;
     localtime_r(then, &tm);
     strftime(date, date_len, "%FT%T", &tm);
}

/* Accept any date that svn_parse_date can handle and reformat the "date" 
   buffer with an ISO8601 formatted date.
   
   Examples:
   * next wednesday at 5pm
   * 10 minutes
   * 5 hours 10 minutes
   * +05:10
   * tomorrow
   */
time_t normalize_date(char *key, char *keyword, char *date, int date_len) 
{
     time_t now, then;
     struct tm *tm;
     now = time (NULL);
	
     if (xapmsg_getvalue(key, date) == 0) {
	  if (g_debuglevel>5) printf("Error: No %s specified in message body\n", keyword);
	  return -1;
     }
     if (svn_parse_date (date, now, &then) != 0) {
	  if (g_debuglevel>5) printf("Error: invalid %s date: '%s'\n", keyword, date);
	  return -1;
     }
     to_iso8601(date, date_len, &then);
     return then;
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
     if (xapmsg_getvalue("xap-header:target", i_target) == 0 ) {
	  if (g_debuglevel>7) printf("No target in header, not for us\n");
	  return;
     }
     if (xapmsg_getvalue("xap-header:source", i_source) == 0 ) {
	  if (g_debuglevel>3) printf("No source in header\n");
	  return;
     }			
     if (xapmsg_getvalue("xap-header:class", i_class) == 0 ) {
	  if (g_debuglevel>3) printf("No class in header\n");
	  return;
     }
     if (strcasecmp(i_class, XAP_CLASS) != 0 ) {
	  if (g_debuglevel>7) printf("Class didn't match %s to %s\n", i_class, XAP_CLASS);
	  return;
     }
	 
     snprintf(i_identity, sizeof(i_identity), "%s.%s.%s", XAP_ME, XAP_SOURCE, g_instance);
     if (xap_compare(i_target, i_identity) != 1 ) {
	  if (g_debuglevel>7) printf("Target didn't match %s to %s\n", i_target, i_identity);
	  return;
     }

     char title[XAP_MAX_KEYVALUE_LEN];
     char start[XAP_MAX_KEYVALUE_LEN];
     char end[XAP_MAX_KEYVALUE_LEN];
     char description[XAP_MAX_KEYVALUE_LEN];

     if (xapmsg_getvalue("event:title", title)==0) {
	  if (g_debuglevel>5) printf("Error: No TITLE specified in message body\n");
	  return;
     }
     // side-affect: start is altered to be an ISO8601 compliant timestamp.
     time_t start_t = normalize_date("event:start","START", start, sizeof(start));
     if(start_t == -1) return;
     if(normalize_date("event:end","END", end, sizeof(end)) == -1) {
	  // End date wasn't valid we'll add 1 min to the start and use that
	  // to correct the error condition.
	  time_t end_t = start_t + EVENT_FREQ;
	  to_iso8601(end, sizeof(end), &end_t);
     }
     if(xapmsg_getvalue("event:description", description) == 0)
       description[0] = 0;

     gcal_event_t event;
     if((event = gcal_event_new(NULL))) {
	  gcal_event_set_title(event, title);
	  gcal_event_set_content(event, description);
	  gcal_event_set_where(event, "xap-event");
	  gcal_event_set_start(event, start);
	  gcal_event_set_end(event, end);
	  if(g_debuglevel) {
	       printf("Send EVENT to Google\n");
	       dump_event(event);
	  }
	  int result = gcal_add_event(gcal, event);
	  // 201 (CREATED)
	  if(!result && g_debuglevel) {
	       /* Check for errors and print status code */
	       printf("Failed adding a new event! HTTP code: %d"
		      "\nmsg: %s\n", gcal_status_httpcode(gcal),
		      gcal_status_msg(gcal));
	  }
	  gcal_event_delete(event);
     }
}

void process_loop() 
{
     fd_set master, i_rdfs;
     char i_xap_buff[1500];   // incoming XAP message buffer
     time_t tick1, tick2, now;
     struct timeval i_tv;
     struct gcal_event_array event_array;
     struct tm loctime;

     // Setup the select() sets.
     FD_ZERO(&master);
     FD_SET(g_xap_receiver_sockfd, &master);

     tick1 = tick2 = 0;
     while(1) {
	  while (loctime.tm_sec) {  // Poll google and check events on the minute boundary.
	       now = time(NULL);
	       localtime_r(&now, &loctime);
			
	       i_tv.tv_sec = EVENT_FREQ - loctime.tm_sec; // Block select for up to ...
	       i_tv.tv_usec=0;		
	       i_rdfs = master; // copy it - select() alters this each call.
	       select(g_xap_receiver_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);
			
	       // if incoming XAP message...
	       // Check for new incoming XAP messages.  They are handled in xap_handler.
	       if (FD_ISSET(g_xap_receiver_sockfd, &i_rdfs)) {
		    if (xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buff, sizeof(i_xap_buff))>0) {
			 xap_handler(i_xap_buff);
		    }
	       }		
	       // Send heartbeat periodically
	       xap_heartbeat_tick(HBEAT_INTERVAL);
	  }

	  // Every user-defined sync period refresh the event list.
	  if ((now-tick1 >= freq)||(tick1==0)) { 
	       if(tick1) gcal_cleanup_events(&event_array);
	       tick1 = now;
	       if(g_debuglevel >= 6) {
		    printf("%d:%02d:%02d - Reloading EVENT cache\n", 
			   loctime.tm_hour, loctime.tm_min, loctime.tm_sec);
	       }
	       get_period_events(&event_array);
	  }    

	  // Every EVENT_FREQ period fire google xap events
	  if ((now-tick2 >= EVENT_FREQ)||(tick2==0)) { 
	       tick2 = now;
	       if(g_debuglevel >= 6) {
		    printf("%d:%02d:%02d - Check Trigger events\n", 
			   loctime.tm_hour, loctime.tm_min, loctime.tm_sec);
	       }
	       fire_google_calendar_event(&event_array);
	  }

	  loctime.tm_sec = 1; // force entry into select() while loop.
     } //while(1)

}

void setupXAPini() 
{
     char uid[5];
     char guid[9];
     long n;

     // default to 00DA if not present
     n = ini_gets("googlecal","uid","00DA",uid,sizeof(uid),inifile);

     // validate that the UID can be read as HEX
     if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
		    isxdigit(uid[2]) && isxdigit(uid[3]))) 
     {
	  strcpy(uid,"00DA");
     }
     snprintf(guid,sizeof guid,"FF%s00", uid);
     XAP_GUID = strdup(guid);

     char control[30];
     n = ini_gets("googlecal","instance","GoogleCal",control,sizeof(control),inifile);
     if(n == 0 || strlen(control) == 0) 
     {
	  strcpy(control,"GoogleCal");
     }
     XAP_DEFAULT_INSTANCE = strdup(control);

     freq = ini_getl("googlecal","ufreq", 60, inifile);
     if(freq < 60 || freq > 60*60*24) freq = 60; // default 60sec - 24hrs
  
     ini_gets("googlecal","user","",username,sizeof(username),inifile);
     if(strlen(username) == 0) {
	  printf("Error: user has not been setup\n");
	  exit(1);
     }

     ini_gets("googlecal","passwd","",password,sizeof(password),inifile);
     if(strlen(password) == 0) {
	  printf("Error: passwd has not been setup\n");
	  exit(1);
     }  
}

int main(int argc, char *argv[]) 
{
     printf("\nGoogle Calendar Connector for xAP v12\n");
     printf("Copyright (C) DBzoo 2009\n\n");
  
     setupXAPini();  
     xap_init(argc, argv, 0);
     /* Create a gcal 'object' and authenticate with server */
     if (!(gcal = gcal_new(GCALENDAR))) 
     {
	  printf("Failed to create Google Calendar Object\n");
	  exit(1);
     }
  
     if(g_debuglevel) printf("Connecting to google\n");
     int result = gcal_get_authentication(gcal, username, password);
     if(result)
     {
	  printf("Failed to authenticate\n");
	  printf("%d: %s\n", gcal_status_httpcode(gcal), gcal_status_msg(gcal));
	  exit(1);
     }
  
     process_loop();
}
