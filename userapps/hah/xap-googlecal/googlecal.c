/* Copyright (c) Brett England, 2009
 
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
   where=
   }
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef DEV
#include <inc/gcalendar.h>
#include <inc/internal_gcal.h>
#include <inc/gcal_status.h>
#else
#include <libgcal/gcalendar.h>
#include <libgcal/internal_gcal.h>
#include <libgcal/gcal_status.h>
#endif
#include "xap.h"
#include "svn_date.h"

// Frequency in seconds that we check for a trigger event.
// As google calendar can only define to the minute this number
// reflects that granulality.
#define EVENT_FREQ 60
#define DEF_POLL_FREQ 60

const char *inifile = "/etc/xap.d/xap-googlecal.ini";

// Commandline / INI settings
static int freq; // calendar sync frequency
static char username[64];
static char *password=NULL;
static char *interfaceName = NULL;

// Internal
static struct gcal_event_array events;
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
void dump_events()
{
        gcal_event_t event;
        int i;
	
        printf("Dump event cache - found %d events\n", events.length);
        for(i=0; i<events.length; i++) {
                event = gcal_event_element(&events, i);
                if(event)
                        dump_event(event);
        }
}

void internalError(int level, const char *fmt, ...)
{
        char buff[XAP_DATA_LEN];
        va_list ap;

        va_start(ap, fmt);
        log_write(level, fmt, ap); // Log it

        int len = sprintf(buff, "xap-header\n"
                          "{\n"
                          "v=12\n"
                          "hop=1\n"
                          "uid=%s\n"
                          "class=google.calendar\n"
                          "source=%s\n"
                          "}\n"
                          "error\n"
                          "{\n"
                          "text=", xapGetUID(), xapGetSource());
        len += vsnprintf(&buff[len], XAP_DATA_LEN-len, fmt, ap);
        va_end(ap);
        len += snprintf(&buff[len], XAP_DATA_LEN-len, "\n}\n");

        if(len < XAP_DATA_LEN) {
                xapSend(buff);
        } else {
                err("Buffer overflow %d/%d", len, XAP_DATA_LEN);
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

/** Get all events - use for debugging
* xAP Timeout CALLBACK
* 
*/
void googleAllEventCheck(int interval, void *userData)
{
        gcal_get_events(gcal, &events);
}

/** Get all events between now and the next polling period
* xAP Timeout CALLBACK
* 
*/
void googlePeriodEventCheck(int interval, void *userData)
{
        char start_timestamp[TIMESTAMP_MAX_SIZE];
        char stop_timestamp[TIMESTAMP_MAX_SIZE];
        int result = -1;
        char query[256];

        // to access gcal members we had to include <internal_gcal.h>
        if(get_offset_timestamp(start_timestamp, TIMESTAMP_MAX_SIZE, 0, gcal->timezone))
                return;
        if(get_offset_timestamp(stop_timestamp, TIMESTAMP_MAX_SIZE, freq, gcal->timezone))
                return;

        // http://code.google.com/apis/calendar/data/2.0/reference.html#Parameters
        snprintf(query, sizeof(query), "start-min=%s&start-max=%s&singleevents=true",
                 start_timestamp, stop_timestamp);
        debug("Google query params: %s", query);

        result = gcal_query(gcal, query, "GData-Version: 2");
        if (result) {
                debug("Google returns no events");
                events.entries = NULL;
                events.length = 0;
                return;
        }

        events.entries = gcal_get_entries(gcal, &events.length);
        if(events.entries)
                return;

	if(getLoglevel() == LOG_DEBUG) {
	  dump_events();
	}
}

/** Search the array of pending calendar events and if its active action it.
* xAP Timeout CALLBACK
*/
void googleEventCheck(int interval, void *userData)
{
        gcal_event_t event;
        int result;
        int i;
        time_t start_time = (time_t)NULL, stop_time = (time_t)NULL;
        time_t curtime = time(NULL);

        for(i=0; i<events.length; i++) {
                event = gcal_event_element(&events, i);
                if(!event)
                        break;

		if(getLoglevel() == LOG_DEBUG) {
		  debug("Trigger event");
		  dump_event(event);
		}

                // Convert the event ISO8601 timestamp - we never expect these to fail...
                char *date = gcal_event_get_start(event);
                if(svn_parse_date(date, curtime, &start_time) != 0) {
                        internalError(LOG_NOTICE, "Error parsing START date: %s", date);
                        continue;
                }

                date = gcal_event_get_end(event);
                if(svn_parse_date(date, curtime, &stop_time) != 0) {
                        internalError(LOG_NOTICE,"Error parsing END date: %s", date);
                        continue;
                }

                // Is the event ready to fire?
                if(start_time > curtime && stop_time < curtime) {
                        info("Event not ready to fire");
                        continue;
                }

                // Only calendar events with the WHERE field = xap are valid.
                // We do this to make life easier for us known that only events
                // marked a certain way are destined for us.  Prevents the users
                // other events from inadvertantly firing us with bad payloads.
                if(strcasecmp("xap",gcal_event_get_where(event))) {
                        info("where<>xap : Not an XAP event");
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
                        // As this is a short xAP message fix it up.
	                xapmsg = fillShortXap(description);
                } else {
                        // pattern match the title against the aliases to find the xAP msg.
                        xapmsg = (char *)malloc(XAP_DATA_LEN);
                        snprintf(xapmsg, XAP_DATA_LEN,
                                 "xap-header\n"
                                 "{\n"
                                 "v=12\n"
                                 "hop=1\n"
                                 "uid=%s\n"
                                 "class=alias\n"
                                 "source=%s\n"
                                 "}\n"
                                 "command\n"
                                 "{\n"
                                 "text=%s\n"
                                 "}\n",
                                 xapGetUID(), xapGetSource(), gcal_event_get_title(event));
                }
                if(xapmsg) {
                        xapSend(xapmsg);
                        free(xapmsg);
                }
        }
}

/// time_t to ISO8601 format - caller needs to free the memory.
char *to_iso8601(time_t *then)
{
        char date[21];
        strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", localtime(then));
        return strdup(date);
}

/** Accept any date that svn_parse_date can handle and reformat the "date"
   buffer with an ISO8601 formatted date.
   
   Examples:
   * next wednesday at 5pm
   * 10 minutes
   * 5 hours 10 minutes
   * +05:10
   * tomorrow
   */
char *normalize_date(char *keyword, char *dateExpr, time_t *then)
{
	if (dateExpr == NULL) return NULL;
        time_t now = time(NULL);
        struct tm *tm;

        if (svn_parse_date (dateExpr, now, then) != 0) {
                internalError(LOG_NOTICE, "unable to parse %s date: '%s'", keyword, dateExpr);
                return NULL;
        }
        return to_iso8601(then);
}

/** Google event recording service
* xAP Filter Action callback
*/
void recordEvent(void *userData)
{
        char *title = xapGetValue("event","title");
        char *start = xapGetValue("event","start");
        char *end = xapGetValue("event","end");
        char *description = xapGetValue("event","description");
        char *where = xapGetValue("event","where");

        time_t startTime;
        time_t endTime;
        char *normalizedStart = normalize_date("START", start, &startTime);
        if(normalizedStart == NULL) {
                return;
        }

        char *normalizedEnd = normalize_date("END", end, &endTime);
        if(normalizedEnd == NULL) {
                info("END not provided defaulting to %d sec from start");
                endTime = startTime + EVENT_FREQ;
                normalizedEnd = to_iso8601(&endTime);
        }

        gcal_event_t event = gcal_event_new(NULL);
        if(event) {
                gcal_event_set_title(event, title);
                gcal_event_set_content(event, description);
                if(where == NULL) {
                        gcal_event_set_where(event, "xap-event");
                } else {
                        gcal_event_set_where(event, where);
                }
                gcal_event_set_start(event, normalizedStart);
                gcal_event_set_end(event, normalizedEnd);

                debug("add a new event");
                if(gcal_add_event(gcal, event) == -1) {
                        /* Check for errors and print status code */
                        internalError(LOG_ERR, "Failed adding a new event");
                        err("HTTP code: %d Msg %s", gcal_status_httpcode(gcal), gcal_status_msg(gcal));
                }
		gcal_event_delete(event); // cleanup (memory free)
        } else {
                internalError(LOG_ERR,"gcal_event_new() failed");
        }

        free(normalizedStart);
        free(normalizedEnd);
}

void validatePollingFrequency(int newFreq) {
	const int secondsInDay = 60*60*24;
	freq = newFreq;
	if(freq < DEF_POLL_FREQ || freq > secondsInDay) {
		freq = DEF_POLL_FREQ;
		warning("Polling frequency out of range defaulting to %d sec", freq);
	}
}

void setupXAPini()
{
	xapInitFromINI("googlecal","dbzoo","GoogleCal","00DA",interfaceName,inifile);
	validatePollingFrequency(ini_getl("googlecal","ufreq", DEF_POLL_FREQ, inifile));
        ini_gets("googlecal","user","",username,sizeof(username),inifile);
	password = getINIPassword("googlecal","passwd", (char *)inifile);
}

/// Display usage information and exit.
static void usage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF\n");
        printf("  -d, --debug            0-7\n");
        printf("  -u, --username\n");
        printf("  -p, --password\n");
        printf("  -f, --freq             Default: %d\n", DEF_POLL_FREQ);
        printf("  -h, --help\n");
        exit(1);
}

int main(int argc, char *argv[])
{
        int i;
        printf("\nGoogle Calendar Connector for xAP v12\n");
        printf("Copyright (C) DBzoo 2009-2010\n");

        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        interfaceName = argv[++i];
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
                        usage(argv[0]);
                }
        }

        setupXAPini();

        // Command line override for INI
        for(i=0; i<argc; i++) {
                if(strcmp("-u", argv[i]) == 0 || strcmp("--username",argv[i]) == 0) {
                        strlcpy(username, argv[++i], sizeof(username));
                } else if(strcmp("-p", argv[i]) == 0 || strcmp("--password",argv[i]) == 0) {
	                if(password) free(password);
                        password = strdup( argv[++i] );
                } else if(strcmp("-f", argv[i]) == 0 || strcmp("--freq",argv[i]) == 0) {
	                validatePollingFrequency(atoi(argv[++i]));
                }
        }

        if(strlen(username) == 0) {
                die("user has not been setup");
        }
        if(strlen(password) == 0) {
                die("passwd has not been setup");
        }

        /* Create a gcal 'object' and authenticate with server */
        gcal = gcal_new(GCALENDAR);
        if (gcal == NULL) {
                internalError(LOG_EMERG, "Failed to create Google Calendar Object!");
                exit(1);
        }

        info("Connecting to google");
        if(gcal_get_authentication(gcal, username, password) == -1) {
                internalError(LOG_ALERT, "Failed to authenticate");
        	alert("HTTP code: %d Msg %s", gcal_status_httpcode(gcal), gcal_status_msg(gcal));

	        time_t now = time(NULL);
	        struct tm *theTime;
	        theTime = gmtime(&now);
	        if(theTime->tm_year+1900 < 2010) { // sanity check the year.
		        printf("Your clock is not set SSL requires it\n");
	        	printf("System time: %s", ctime(&now));
	        }
	        exit(1);
        }

        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", "google.calendar");
        xapAddFilter(&f, "event", "title", XAP_FILTER_ANY);
        xapAddFilter(&f, "event", "start", XAP_FILTER_ANY);
        xapAddFilterAction(&recordEvent, f, NULL);

        xapAddTimeoutAction(&googleEventCheck, EVENT_FREQ, NULL);
        xapAddTimeoutAction(&googlePeriodEventCheck, freq, NULL);
        //xapAddTimeoutAction(&googleAllEventCheck, freq, NULL);

        xapProcess();
}
