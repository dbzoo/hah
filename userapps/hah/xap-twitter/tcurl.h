/**
 $Id$

 Brett England
 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.  
*/
#ifndef _TWITCURL_H_
#define _TWITCURL_H_

#include <curl/curl.h>

#define TWITCURL_URL_LEN 512

typedef struct _tcurl {
  CURL *curlHandle;
  char errorBuffer[CURL_ERROR_SIZE];

  int cb_length; // Current size of callback data buffer
  char *callbackData;

  char url[TWITCURL_URL_LEN];
  char *user; // screen name
  char *userid;
  char *oauthAccessKey;
  char *oauthAccessSecret;
} tcurl;

extern char *CONSUMER_KEY;
extern char *CONSUMER_SECRET;

tcurl *new_tcurl();
void free_tcurl(tcurl *c);
int userGet(tcurl *c, char *userInfo, int isUserId);
char *getLastWebResponse(tcurl *c);
#endif
