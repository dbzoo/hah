/**
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xap.h"
#include "bsc.h"
#include "tcurl.h"

char *inifile = "/etc/xap.d/xap-twitter.ini";
char *interfaceName = NULL;

static int freq; // calendar sync frequency

char *CONSUMER_KEY;
char *CONSUMER_SECRET;
char access_key[128];
char access_secret[128];
char user[32];
char userid[16];
char prefix[20];
int prefix_len = 0;

static tcurl *twit;

// A "long long" on mips is 8 bytes plenty large enough for our ID.
static long long last_tweet; // ID of the last TWEET processed.
// Current TWEET details
static char tweet[141];

/**
 * Periodically connect to twitter grab the latest tweet and compare its ID
 * to the one previously received.  If its later process it.
 */
void checkForTweets(int interval, void *userData)
{
	// Check for error getting tweet.
	if( getLatestTweet(twit, tweet, sizeof(tweet), &last_tweet) == -1) return;
	// Check tweets command prefix, if applicable.
	if(!(prefix_len == 0 || strncasecmp(tweet, prefix, prefix_len) == 0)) return;
		
	char *tweeter = &tweet[0];	
	if (prefix_len)
		tweeter += prefix_len;
	
                // Send the alias in an xAP message.
	char buff[XAP_DATA_LEN];
	int len = snprintf(buff, sizeof(buff),
	               "xap-header\n{\nv=12\nhop=1\nuid=%s\n"
	               "class=alias\nsource=%s\n}\n"
	               "command\n{\ntext=%s\n}\n",
	                   xapGetUID(), xapGetSource(), tweeter);

	// Buffer overflow - should never happend with 140 char tweets.
	if(len > sizeof(buff)) {
		err("Buffer overflow sending %s", buff);
		return;
	}
	xapSend(buff);
	deleteTweetById(twit, last_tweet);
}

/// xAP message service to send a tweet
void tweetService(void *userData)
{
        // As this is not a REAL BSC endpoint as we only handle a single output section.
        // A BSC endpoint would also emit an INFO/EVENT when a CMD is seen we don't do that either.
        // This should probably not be using BSC!  Leave it for now...

        char *text = xapGetValue("output.state.1","text");
        if(sendTweet(twit, text) == 0) {
		// Get it back to update the ID so its not deleted!
		getLatestTweet(twit, tweet, sizeof(tweet), &last_tweet);
	}
}

void setupXAPini()
{
        xapInitFromINI("twitter","dbzoo","Twitter","00D9",interfaceName,inifile);

        //Rate limit exceeded. Clients may not make more than 150 requests per hour.
        freq = ini_getl("twitter","ufreq", 30, inifile);
        if(freq < 10 || freq > 60*5)
                freq = 30;

        ini_gets("twitter","prefix","",prefix,sizeof(prefix),inifile);
        prefix_len = strlen(prefix);

        // PREFIX command always has a trailing space.
        if (prefix_len) {
                prefix[prefix_len++] = ' ';
                prefix[prefix_len] = '\0';
        }

        ini_gets("twitter","access_key","",access_key,sizeof(access_key),inifile);
        ini_gets("twitter","access_secret","",access_secret,sizeof(access_secret),inifile);
        char consumer_key[128];
        char consumer_secret[128];
        ini_gets("twitter","consumer_key","",consumer_key,sizeof(consumer_key),inifile);
        ini_gets("twitter","consumer_secret","",consumer_secret,sizeof(consumer_secret),inifile);
        CONSUMER_KEY = strdup(consumer_key);
        CONSUMER_SECRET = strdup(consumer_secret);
        ini_gets("twitter","user","",user,sizeof(user),inifile);
        ini_gets("twitter","userid","",userid,sizeof(userid),inifile);

        if(strlen(access_key) == 0 ||
                        strlen(access_secret) == 0 ||
                        strlen(consumer_key) == 0 ||
                        strlen(consumer_secret) == 0 ||
                        strlen(user) == 0 ||
                        strlen(userid) == 0) {
                die("Twitter not authorized");
        }
}


int main(int argc, char *argv[])
{
        printf("\nTwitter Connector for xAP v12\n");
        printf("Copyright (C) DBzoo 2009\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);	
        setupXAPini();

	twit = new_tcurl();
	die_if(twit == NULL,"Failed to create Twitter CURL Object");
	
        twit->oauthAccessKey = access_key;
        twit->oauthAccessSecret = access_secret;
        twit->user = user;
        twit->userid = userid;

        getLatestTweet(twit, tweet, sizeof(tweet), &last_tweet);

        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", BSC_CMD_CLASS);
        xapAddFilter(&f, "output.state.1", "text", XAP_FILTER_ANY);
        xapAddFilterAction(&tweetService, f, NULL);

        xapAddTimeoutAction(&checkForTweets, freq, NULL);

        xapProcess();
}
