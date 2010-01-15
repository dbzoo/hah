/**
 $Id$
 
 XAP Twitter daemon

 This daemon will monitor a twitter feed scanning for command that match the preconfigured
 aliases. When a match is found the xAP will be sent and the TWEET will DELETED.

 It also allows you to send tweets via XAP.

 xap-header
 {
 target=dbzoo.livebox.twitter
 class=xAPBSC.cmd
 }
 output.state.1
 {
 id=*
 text=This is an xAP initiated tweet
 }

 Brett England
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
#include "xapdef.h"
#include "xapGlobals.h"
#ifdef XMLALIAS
#include "alias.h"
#endif
#include "tcurl.h"

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

static int freq; // calendar sync frequency
static char username[64];
static char password[64];

#ifdef XMLALIAS
const char defalias[] = "/etc/xap-alias.xml";
static alias_t *aliases;
static char aliasfile[64];
#endif
const char inifile[] = "/etc/xap-livebox.ini";

static tcurl *twit;

// A "long long" on mips is 8 bytes plenty large enough for our ID.
static long long last_tweet; // ID of the last TWEET processed.
// Current TWEET details
static char tweet[141];
static long long id;

void process_tweet() {
#ifdef XMLALIAS
     // Use the XML file to transform the alias into a valid xAP message.
     char *xapmsg = matchAlias(aliases, tweet);
     if(xapmsg) {
	  if(g_debuglevel) printf("Send XAP message\n%s\n", xapmsg);
	  xap_send_message(xapmsg);
	  free(xapmsg);
	  // Twitter won't let you send the same tweet twice within a given time period.
	  // As we are using twitter for a control interface the workaround it to delete
	  // a tweet when the livebox processes it.
	  // This gives active feedback it was actioned and allows it to be sent again.
	  deleteTweetById(twit, id);
     }
#else
     // Send the alias in an xAP message.
     char buff[1500];
     int len;
     len = snprintf(buff, sizeof(buff),
		    "xap-header\n{\nv=12\nhop=1\nuid=%s\n"
		    "class=alias\nsource=%s.%s.%s\n}\n"
		    "command\n{\ntext=%s\n}\n", 
		    g_uid, XAP_ME, XAP_SOURCE, g_instance, tweet);
     
     if(len > sizeof(buff)) // Buffer overflow!
	  return 0;
     xap_send_message(buff);
     deleteTweetById(twit, id);
#endif
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
     if (strcasecmp(i_class, "xapbsc.cmd") != 0 ) {
	  if (g_debuglevel>7) printf("Class didn't match xAPBSC.cmd to %s\n", i_class);
	  return;
     }

     snprintf(i_identity, sizeof(i_identity), "%s.%s.%s", XAP_ME, XAP_SOURCE, g_instance);
     if (xap_compare(i_target, i_identity) != 1 ) {
	  if (g_debuglevel>7) printf("Target didn't match %s to %s\n", i_target, i_identity);
	  return;
     }

     int i;
     for(i=0; i<g_xap_index; i++) {
	     struct tg_xap_msg *map = &g_xap_msg[i];
	     if(! xap_compare(map->section,"output.state.*")) continue;
	     if(strcasecmp(map->name,"text") == 0) {
		     if(g_debuglevel) printf("send tweet: %s\n", map->value);
		     sendTweet(twit, map->value);
	     }
     }
}

/** 
 * Periodically connect to twitter grab the latest tweet and compare its ID
 * to the one previously received.  If its later process it against our alias cmds.
 * If an xAP message is recieved check if we need to handle it.
 */
void process_loop() 
{
	int ret;
	char i_xap_buff[1500];
	struct timeval i_tv;
	fd_set master, i_rdfs;	

	// Setup the select() sets.
	FD_ZERO(&master);
	FD_SET(g_xap_receiver_sockfd, &master);

	i_tv.tv_sec = freq;
	i_tv.tv_usec = 0;
	while(1) {
		i_rdfs = master;
		// On Linux, select() modifies timeout to reflect the amount of time not
		// slept; most other implementations do not do this

		ret = select(g_xap_receiver_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);

		// As select() is guarantee'd to receive somebody else hearbeat it will unblock
		// at least every HBEAT_INTERVAL allowing our's to be sent too.

		if (ret == -1) { // if error
			if(g_debuglevel) perror("select()");
		} else if(ret) { // if incoming XAP message...
			if (xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buff, sizeof(i_xap_buff))>0) {
				xap_handler(i_xap_buff);
			}
		} else { // timeout
			getLatestTweet(twit, tweet, sizeof(tweet), &id);
			if(id > last_tweet) {
				process_tweet();
				last_tweet = id;
			}
			i_tv.tv_sec = freq;
			i_tv.tv_usec = 0;
		}
		// Send heartbeat periodically
		xap_heartbeat_tick(HBEAT_INTERVAL);
	}
}

void setupXAPini() 
{
	char uid[5];
	char guid[9];
	long n;

	// default to 00D9 if not present
	n = ini_gets("twitter","uid","00D9",uid,sizeof(uid),inifile);

	// validate that the UID can be read as HEX
	if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
		       isxdigit(uid[2]) && isxdigit(uid[3]))) 
	{
		strcpy(uid,"00D9");
	}
	snprintf(guid,sizeof guid,"FF%s00", uid);
	XAP_GUID = strdup(guid);

	char control[20];
	n = ini_gets("twitter","instance","Twitter",control,sizeof(control),inifile);
	if(n == 0 || strlen(control) == 0) 
	{
		strcpy(control,"Twitter");
	}
	XAP_DEFAULT_INSTANCE = strdup(control);

	//Rate limit exceeded. Clients may not make more than 150 requests per hour.
	freq = ini_getl("twitter","ufreq", 30, inifile);
	if(freq < 30 || freq > 60*5) freq = 30;
  
	ini_gets("twitter","user","",username,sizeof(username),inifile);
	if(strlen(username) == 0) {
		printf("Error: user has not been setup\n");
		exit(1);
	}

	ini_gets("twitter","passwd","",password,sizeof(password),inifile);
	if(strlen(password) == 0) {
		printf("Error: passwd has not been setup\n");
		exit(1);
	}  
#ifdef XMLALIAS
	ini_gets("twitter","aliasfile",defalias,aliasfile,sizeof(aliasfile),inifile);
#endif
}


int main(int argc, char *argv[]) 
{
	printf("\nTwitter Connector for xAP v12\n");
	printf("Copyright (C) DBzoo 2009\n\n");
  
	setupXAPini();  
	xap_init(argc, argv, 0);
#ifdef XMLALIAS
	if(g_debuglevel) printf("Reading aliases from %s\n", aliasfile);
	aliases = parseAliasDoc(aliasfile);
	if(g_debuglevel) dumpAliases(aliases);
	if(NULL == aliases) {
		printf("No aliases defined\n");
		exit(1);
	}
#endif
	if (!(twit = new_tcurl())) 
	{
		printf("Failed to create Twitter CURL Object\n");
		exit(1);
	}

	int result = tcurl_get_authentication(twit, username, password);
	if(result) {
		printf("Failed to authenticate\n");
		exit(1);
	}

	getLatestTweet(twit, tweet, sizeof(tweet), &last_tweet);
	process_loop();
}
