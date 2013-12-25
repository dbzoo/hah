/* deleteLastTweet.c
 * Test the functionality of the fetch and delete calls.
 * Does what it says on the tin.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xap.h"
#include "tcurl.h"

char *inifile = "/etc/xap.d/xap-twitter.ini";
char *interfaceName = "eth0";

char *CONSUMER_KEY;
char *CONSUMER_SECRET;
char access_key[128];
char access_secret[128];
char user[32];
char userid[16];

static tcurl *twit;

// A "long long" on mips is 8 bytes plenty large enough for our ID.
static long long last_tweet; // ID of the last TWEET processed.
// Current TWEET details
static char tweet[141];

int main(int argc, char *argv[])
{
        printf("\nDelete latest tweet\n");
        printf("Copyright (C) DBzoo 2012\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);	

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

	twit = new_tcurl();
	die_if(twit == NULL,"Failed to create Twitter CURL Object");
	
        twit->oauthAccessKey = access_key;
        twit->oauthAccessSecret = access_secret;
        twit->user = user;
        twit->userid = userid;

        getLatestTweet(twit, tweet, sizeof(tweet), &last_tweet);
	deleteTweetById(twit, last_tweet);
}
