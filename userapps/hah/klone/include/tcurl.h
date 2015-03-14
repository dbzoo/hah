/*
 Brett England
 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.  
*/
#ifndef _TWITCURL_H_
#define _TWITCURL_H_

#include <curl/curl.h>
#include <klone/io.h>

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
  io_t *out;
} tcurl;

extern const char *CONSUMER_KEY;
extern const char *CONSUMER_SECRET;

tcurl *new_tcurl(io_t *io);
void free_tcurl(tcurl *c);
int userGet(tcurl *c, char *userInfo, int isUserId);
char *getLastWebResponse(tcurl *c);
char *oauthGetAuthorizeUrl(tcurl *c, char *callbackURL);
int oauthParseReply(char *reply, char **token, char **secret, char **user, char **userid);
int oauthAuthorize(tcurl *c, char *oauth_verify);
#endif
