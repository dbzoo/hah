/**
   $Id$
 
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
#include "oauth.h"
#include "log.h"

#ifdef IDENT
#ident "@(#) $Id$"
#endif

static void clearCallbackBuffers(tcurl *c) {
	debug("len: %d", c->cb_length);
	memset( c->errorBuffer, 0, CURL_ERROR_SIZE);
	memset( c->callbackData, 0, c->cb_length);
}

inline char *getLastCurlErr(tcurl *c) {
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
                        crit("Failed relloc!");
                        goto exit;
                }
                c->callbackData = ptr_tmp;
        }
        strncat(c->callbackData, (char *)ptr, size);
exit:
        return size;
}


tcurl *new_tcurl() {
	tcurl *c = (tcurl *)calloc(1, sizeof(tcurl));

        // Set resonable sizes for our buffers; these will realloc if necessary.
        c->callbackData = (char *)malloc(256);
        c->cb_length = 256;
        c->curlHandle = curl_easy_init();
        clearCallbackBuffers(c);

        if (NULL == c->curlHandle) {
		crit("Fail to init CURL");
        } else {
		curl_easy_setopt( c->curlHandle, CURLOPT_FAILONERROR, 1);
		curl_easy_setopt( c->curlHandle, CURLOPT_FOLLOWLOCATION, 1);

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
	long responseCode = 0;
	debug("%s", c->url);
        clearCallbackBuffers(c);

        char *req_url_signed = oauth_sign_url2(c->url, NULL, OA_HMAC, NULL, 
                                               CONSUMER_KEY, CONSUMER_SECRET, 
                                               c->oauthAccessKey, c->oauthAccessSecret);

	if(req_url_signed) {
		curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 1);
		curl_easy_setopt( c->curlHandle, CURLOPT_URL, req_url_signed);
		if(getLoglevel() == LOG_DEBUG) {
			curl_easy_setopt( c->curlHandle, CURLOPT_VERBOSE, 1 );
		}

		CURLcode code = curl_easy_perform(c->curlHandle);

		curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 0);
		free(req_url_signed);
		
		switch(code) {
		case CURLE_OK: 
			return 0;
		case CURLE_HTTP_RETURNED_ERROR:
			curl_easy_getinfo(c->curlHandle, CURLINFO_HTTP_CODE, &responseCode);
			err("### response=%ld", responseCode);
			break;
		default:
			err("curlCode: %i  msg: %s", code, curl_easy_strerror(code));
		}
	} else {
		err("request URL was not signed");
	}
        return -1;
}

static int performPost(tcurl *c, char *url, char *msg) {
	long responseCode = 0;
        struct curl_slist *headerlist=NULL;
	char *req_url = NULL;
	char *req_hdr = NULL;
	char *http_hdr = NULL;
        char *postarg = NULL;
	int argc;
	char **argv = NULL;

	debug("url=%s", url);
	debug("msg=%s", msg);
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
		
        curl_easy_setopt( c->curlHandle, CURLOPT_POST, 1 );
        curl_easy_setopt( c->curlHandle, CURLOPT_URL, url );
	curl_easy_setopt( c->curlHandle, CURLOPT_POSTFIELDS, msg );
	if(getLoglevel() == LOG_DEBUG) {
		curl_easy_setopt( c->curlHandle, CURLOPT_VERBOSE, 1 );
	}

        headerlist = curl_slist_append(headerlist, "Expect:"); // Disable
	http_hdr = malloc(strlen(req_hdr) + 55);
	sprintf(http_hdr, "Authorization: OAuth %s", req_hdr);
        headerlist = curl_slist_append(headerlist, http_hdr); // Add OAUTH header..
        curl_easy_setopt( c->curlHandle, CURLOPT_HTTPHEADER, headerlist);

        CURLcode code = curl_easy_perform(c->curlHandle);

	// CLEAN UP
        curl_easy_setopt( c->curlHandle, CURLOPT_POST, 0);
        curl_easy_setopt( c->curlHandle, CURLOPT_POSTFIELDS, 0 );
        curl_easy_setopt( c->curlHandle, CURLOPT_HTTPHEADER, 0);

	oauth_free_array(&argc, &argv);
        curl_slist_free_all(headerlist);

	if (req_hdr) free(req_hdr);
	if (req_url) free(req_url);
	if (http_hdr) free(http_hdr);
	if (postarg) free(postarg);

	switch(code) {
	case CURLE_OK: 
		return 0;
	case CURLE_HTTP_RETURNED_ERROR:
		curl_easy_getinfo(c->curlHandle, CURLINFO_HTTP_CODE, &responseCode);
		err("### response=%ld", responseCode);
		break;
	default:
		err("curlCode: %i  msg: %s", code, curl_easy_strerror(code));
	}
        return -1;
}

static int performDelete(tcurl *c) {
	int argc;
	char **argv = NULL;
        clearCallbackBuffers(c);

	argc = oauth_split_url_parameters(c->url, &argv);	
	char *req_url_signed = oauth_sign_array2(&argc, &argv, 
						 NULL,  // postarg
						 OA_HMAC, "DELETE",
						 CONSUMER_KEY, CONSUMER_SECRET, 
						 c->oauthAccessKey, c->oauthAccessSecret);
	oauth_free_array(&argc, &argv);

        curl_easy_setopt( c->curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt( c->curlHandle, CURLOPT_URL, req_url_signed);
	if(getLoglevel() == LOG_DEBUG)
		curl_easy_setopt( c->curlHandle, CURLOPT_VERBOSE, 1 );

        int ret = CURLE_OK == curl_easy_perform(c->curlHandle);
        curl_easy_setopt( c->curlHandle, CURLOPT_CUSTOMREQUEST, 0);

        free(req_url_signed);
        return ret;
}

inline char *getLastWebResponse( tcurl *c )
{
        return c->callbackData;
}

 
int deleteTweetById( tcurl *c, long long id ) {
        int retVal = -1;
        if( isCurlInit(c) )
        {
                snprintf(c->url, TWITCURL_URL_LEN, "http://twitter.com/statuses/destroy/%lld.xml", id);
                retVal = performDelete( c );
        }
        return retVal;
}

int userGet(tcurl *c, char *userInfo, int isUserId) {
        int retVal;
        if( isCurlInit(c) && userInfo) {
                strcpy(c->url, "http://twitter.com/users/show.xml");
                strcat(c->url, isUserId ? "?user_id=" : "?screen_name=");
                strcat(c->url, userInfo);
                retVal = performGet(c);
        }
        return retVal;
}

// Get the latest tweet along with when it was published.  We assume for the authenticated user.
int getLatestTweet(tcurl *c, char *content, int clen, long long *id)
{
	debug("Get tweet after this ID %lld", *id);

	if (*id > 0) { // Get tweet after this POINT
		snprintf(c->url, TWITCURL_URL_LEN,"http://api.twitter.com/1/statuses/user_timeline.xml?user_id=%s&since_id=%lld&trim_user=1&count=1", c->userid, *id);
	} else { // Get the latest
		//snprintf(c->url, TWITCURL_URL_LEN,"http://api.twitter.com/1/statuses/user_timeline.xml?user_id=%s&trim_user=1&count=1", c->userid);
		snprintf(c->url, TWITCURL_URL_LEN,"http://192.168.1.9/twitter");
	}

	if(performGet(c)) {
		return -1;
	}
        char *result = getLastWebResponse(c);
	debug("Response from TWITTER '%s'", result);

	// An empty STATUS response in XML is 75 characters.
	if(strlen(result) < 76) {
		return -1;
	}

	char *bid = strstr(result,"<id>");
	if(bid == NULL) {
		err("Failed to find <id> tag");
		return -1;
	}
	char *eid = strstr(result,"</id>");
	if(eid == NULL) {
		err("Failed to find </id> tag");
		return -1;
	}
	char *btext = strstr(result,"<text>");
	if(btext == NULL) {
		err("Failed to find <text> tag");
		return -1;
	}
	char *etext = strstr(result,"</text>");
	if(etext == NULL) {
		err("Failed to find </text> tag");
		return -1;
	}

	*eid = 0;
	*etext = 0;
	*id = atoll(bid + 4);
	strlcpy(content, btext + 6, clen);
	info("Tweet: %lld %s", *id, content);

        return 0;
}

int sendTweet(tcurl *c, char *tweet) {
	info("%s", tweet);
        int retVal = -1;
        if( isCurlInit(c) )
        {
                char msg[140+8];
                strcpy(msg, "status=");
                strlcat(msg, tweet, sizeof(msg));
                retVal = performPost( c, "http://api.twitter.com/1/statuses/update.xml", msg );
        }
        return retVal;
}
