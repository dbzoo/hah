/**
   $Id: tcurl.c 10 2010-01-15 18:44:18Z dbzoo.com $
 
   Minimalist Twitter interface

   Brett England
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "tcurl.h"
#include <errno.h>
#include <oauth.h>

#ifdef IDENT
#ident "@(#) $Id: tcurl.c 10 2010-01-15 18:44:18Z dbzoo.com $"
#endif

static void clearCallbackBuffers(tcurl *c) {
        memset( c->errorBuffer, 0, CURL_ERROR_SIZE);
        memset( c->callbackData, 0, c->cb_length);
}

inline char *getLastCurlError(tcurl *c) {
        return c->errorBuffer;
}

static size_t write_cb(void *ptr, size_t count, size_t chunk_size, void *data)
{
        size_t size = count * chunk_size;
        tcurl *c = (tcurl *)data;
        int current_length = strlen(c->callbackData);
        char *ptr_tmp;
  
        if (size > (c->cb_length - current_length - 1)) {
                c->cb_length = current_length + size + 1;
                ptr_tmp = (char *)realloc(c->callbackData, c->cb_length);
                if (!ptr_tmp) {
                        io_printf(c->out,"<b>write_cb: Failed relloc!</b>");
                        goto exit;
                }
                c->callbackData = ptr_tmp;
        }
        strncat(c->callbackData, (char *)ptr, size);
exit:
        return size;
}


tcurl *new_tcurl(io_t *io) {
        tcurl *c = (tcurl *)malloc(sizeof(tcurl));

        clearCallbackBuffers(c);
        // Set resonable sizes for our buffers; these will realloc if necessary.
        c->callbackData = (char *)malloc(256);
        c->cb_length = 256;
        c->curlHandle = curl_easy_init();
        c->oauthAccessKey = NULL;
        c->oauthAccessSecret = NULL;
	c->out = io;

        if (NULL == c->curlHandle) {
		io_printf(io, "<p>Fail to init CURL</p>");
        } else {
		/* Set buffer to get error */
		curl_easy_setopt( c->curlHandle, CURLOPT_ERRORBUFFER, c->errorBuffer );
		
		/* Set callback function to get response */
		curl_easy_setopt( c->curlHandle, CURLOPT_WRITEFUNCTION, write_cb );
		curl_easy_setopt( c->curlHandle, CURLOPT_WRITEDATA, (void * )c);
        }

        return c;
}

static inline int isCurlInit(tcurl *c) {
        return NULL != c->curlHandle ? 1 : 0;
}

void free_tcurl(tcurl *c) {
        if(c == NULL) return;
        if(c->curlHandle) curl_easy_cleanup(c->curlHandle);
	if(c->oauthAccessKey) free(c->oauthAccessKey);
	if(c->oauthAccessSecret) free(c->oauthAccessSecret);
        free(c);
}

int performGet(tcurl *c) {
        clearCallbackBuffers(c);

        char *req_url_signed = oauth_sign_url2(c->url, NULL, OA_HMAC, NULL, 
                                               CONSUMER_KEY, CONSUMER_SECRET, 
                                               c->oauthAccessKey, c->oauthAccessSecret);

        curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 1);
        curl_easy_setopt( c->curlHandle, CURLOPT_URL, req_url_signed);


        int ret= CURLE_OK == curl_easy_perform(c->curlHandle);
        curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 0);

        free(req_url_signed);
        return ret;
}

static int performPost(tcurl *c, char *url, char *msg) {
        struct curl_slist *headerlist=NULL;
	char *req_url = NULL;
	char *req_hdr = NULL;
	char *http_hdr = NULL;
        char *postarg = NULL;
	int argc;
	char **argv = NULL;

        clearCallbackBuffers(c);

	argc = 2;
	argv = (char **)malloc(sizeof(char *)*argc);
	argv[0] = strdup(url);
	argv[1] = strdup(msg);

	oauth_sign_array2_process(&argc, &argv, 
				  &postarg,
				  OA_HMAC, "POST",
				  CONSUMER_KEY, CONSUMER_SECRET, 
				  c->oauthAccessKey, c->oauthAccessSecret);

	req_hdr = oauth_serialize_url_sep(argc, 1, argv, ", ", 6);
	req_url = oauth_serialize_url_sep(argc, 0, argv, "&", 1);
	
	oauth_free_array(&argc, &argv);
	
        curl_easy_setopt( c->curlHandle, CURLOPT_POST, 1 );
        curl_easy_setopt( c->curlHandle, CURLOPT_URL, url );
	curl_easy_setopt( c->curlHandle, CURLOPT_POSTFIELDS, msg );


        headerlist = curl_slist_append(headerlist, "Expect:"); // Disable
	http_hdr = malloc(strlen(req_hdr) + 55);
	sprintf(http_hdr, "Authorization: OAuth %s", req_hdr);
        headerlist = curl_slist_append(headerlist, http_hdr); // Add OAUTH header..
        curl_easy_setopt( c->curlHandle, CURLOPT_HTTPHEADER, headerlist);

        int ret = CURLE_OK == curl_easy_perform(c->curlHandle);

	// CLEAN UP
        curl_easy_setopt( c->curlHandle, CURLOPT_POST, 0);
        curl_easy_setopt( c->curlHandle, CURLOPT_POSTFIELDS, 0 );
        curl_slist_free_all(headerlist);
	if (req_hdr) free(req_hdr);
	if (req_url) free(req_url);
	if (http_hdr) free(http_hdr);
	if (postarg) free(postarg);

        return ret;
}

inline char *getLastWebResponse( tcurl *c )
{
        return c->callbackData;
}

/*
 * request authorize url from twitter.
 * output: authorization url
 */
char *oauthGetAuthorizeUrl(tcurl *c, char *callbackURL) {
        char *req_url = NULL;
        char *postarg = NULL;
        char *t_key = NULL;
        char *t_secret = NULL;
        
        if( isCurlInit(c) )
        {
                // HTTP GET
                c->oauthAccessKey = NULL;
                c->oauthAccessSecret = NULL;

                sprintf(c->url, "http://api.twitter.com/oauth/request_token?oauth_callback=%s", callbackURL);
                performGet(c);
        
                char *reply = getLastWebResponse(c);
        
                if (reply == NULL || *reply == 0) {
		  io_printf(c->out,"<p>Unable to acquire request token from Twitter</p>");
		  return NULL;
                }
                
                if (oauthParseReply(reply, &t_key, &t_secret, NULL, NULL)) {
		  io_printf(c->out,"<p>Unable to parse data from twitter. Content:<pre>");
		  io_printf(c->out,reply);
		  io_printf(c->out,"</pre></p>");
		  return NULL;
                }
        
                /* Compile url string */
                char *t_url = (char *)malloc(50 + strlen(t_key));
                strcpy(t_url,"https://twitter.com/oauth/authorize?oauth_token=");
                strcat(t_url, t_key);
                
                /* Set temporary access key/secret */
                c->oauthAccessKey = t_key;
                c->oauthAccessSecret = t_secret;
                
                return t_url;
        }
        
        return NULL;
}

/*
 * Method to authorize with twitter, and get/store access key/secret.
 *
 * input: PIN for authorization
 * output: bool for success
 */
int oauthAuthorize(tcurl *c, char *oauth_verify) {
        char *t_key;
        char *t_secret;
        char *t_user = NULL;
	char *t_userid = NULL;
  
        if( isCurlInit(c) )
        {
                strcpy(c->url, "http://api.twitter.com/oauth/access_token?oauth_verifier=");
                strcat(c->url, oauth_verify);      
                if(! performGet(c)) return -1;
      
                char *reply = getLastWebResponse(c);
                if (reply == NULL || *reply == 0) {
		  io_printf(c->out,"<p>Unable to get authorisation URL from Twitter.</p>");
		  return -1; 
                }
                if (oauthParseReply(reply, &t_key, &t_secret, &t_user, &t_userid)) {
		  io_printf(c->out,"<p>Unable to parse data from twitter. Content:<pre>");
		  io_printf(c->out,reply);
		  io_printf(c->out,"</pre></p>");
		  return -1; 
                }
      
                /* Set final access key/secret */
                c->oauthAccessKey = t_key;
                c->oauthAccessSecret = t_secret;
                c->user = t_user;
		c->userid = t_userid;
      
                return 0;
        }
        return -1;
}

/*
 * method to parse reply from oauth. this is an internal method.
 * users should not use this method.
 *
 * input: reply - reply data,
 *        token - buffer for oauth key
 *        secret - buffer for oauth secret
 *        screen_name - buffer for twitter username
 *        user_id - buffer for twitter user ID (long as char)
 */
int oauthParseReply(char *reply, char **token, char **secret, char **screen_name, char **user_id) {
        int rc;
        int ok=1;
        char **rv = NULL;
        int i;
	
        rc = oauth_split_post_paramters(reply, &rv, 1);
        qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
        int oauth_cnt = 0;
        for(i=0; i<rc; i++) {
          if(strncmp(rv[i],"oauth_token=",12) == 0) {
            if(token) *token = strdup(&(rv[i][12]));
            oauth_cnt++;
          } else if(strncmp(rv[i],"oauth_token_secret=",19) == 0) {
            if(secret) *secret = strdup(&(rv[i][19]));
            oauth_cnt++;
          } else if(strncmp(rv[i],"screen_name=",12) == 0) {
            if(screen_name) *screen_name = strdup(&(rv[i][12]));
          } else if(strncmp(rv[i],"user_id=",8) == 0) {
            if(user_id) *user_id = strdup(&(rv[i][8]));
          }
        }
        if(rv)
                free(rv);
        if(oauth_cnt >= 2)
                ok = 0;
        else { // Partial deocde?  Cleanup and get outta here...
                free(*token);
                *token = NULL;
                free(*secret);
                *secret = NULL;
		if(screen_name) {
		  free(*screen_name);
		  *screen_name = NULL;
		}
		if(user_id) {
		  free(*user_id);
		  *user_id = NULL;
		}
        }
        return ok;
}
