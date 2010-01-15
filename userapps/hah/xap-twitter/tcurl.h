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

#define TWITCURL_URL_LEN 128

typedef struct _tcurl {
  CURL *curlHandle;
  char errorBuffer[CURL_ERROR_SIZE];

  int cb_length; // Current size of callback data buffer
  char *callbackData;

  char *user;
  char *cred;
  char url[TWITCURL_URL_LEN];
} tcurl;

tcurl *new_tcurl(void);
int tcurl_get_authentication(tcurl *c, char *user, char *passwd);
void free_tcurl(tcurl *c);
int userGet(tcurl *c, char *userInfo, int isUserId);
char *getLastWebResponse(tcurl *c);
int search( tcurl *c, char *query );
int deleteTweetById( tcurl *c, long long id );
int sendTweet(tcurl *c, char *tweet);
int getLatestTweet(tcurl *c, char *content, int clen, long long *id);

#endif
