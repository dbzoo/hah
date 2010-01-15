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
#include "xapdef.h"
#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifdef IDENT
#ident "@(#) $Id$"
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
			if(g_debuglevel) printf("write_cb: Failed relloc!\n");
			goto exit;
		}
		c->callbackData = ptr_tmp;
	}
	strncat(c->callbackData, (char *)ptr, size);
 exit:
	return size;
}


tcurl *new_tcurl() {
	tcurl *c = (tcurl *)malloc(sizeof(tcurl));

	clearCallbackBuffers(c);
	// Set resonable sizes for our buffers; these will realloc if necessary.
	c->callbackData = (char *)malloc(256);
	c->cb_length = 256;
	c->curlHandle = curl_easy_init();

	if (NULL == c->curlHandle) {
		strcpy(c->errorBuffer, "Fail to init CURL");
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

int tcurl_get_authentication(tcurl *c, char *user, char *passwd) {
	int ret = -1;
	if(isCurlInit(c)) {
		c->user = strdup(user);
		c->cred = (char *)malloc(strlen(user) + strlen(passwd) + 2);
		strcpy(c->cred, user);
		strcat(c->cred, ":");
		strcat(c->cred, passwd);
		curl_easy_setopt( c->curlHandle, CURLOPT_USERPWD, c->cred);
		ret = 0;
	}
	return ret;
}

void free_tcurl(tcurl *c) {
	if(c == NULL) return;
	if(c->curlHandle) curl_easy_cleanup(c->curlHandle);
	if(c->cred) free(c->cred);
	free(c);
}

static int performGet(tcurl *c) {
	clearCallbackBuffers(c);
	curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 1);
	curl_easy_setopt( c->curlHandle, CURLOPT_URL, c->url);

	int ret= CURLE_OK == curl_easy_perform(c->curlHandle);
	curl_easy_setopt( c->curlHandle, CURLOPT_HTTPGET, 0);
	return ret;
}

static int performDelete(tcurl *c) {
	clearCallbackBuffers(c);
	curl_easy_setopt( c->curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt( c->curlHandle, CURLOPT_URL, c->url);

	int ret = CURLE_OK == curl_easy_perform(c->curlHandle);
	curl_easy_setopt( c->curlHandle, CURLOPT_CUSTOMREQUEST, 0);
	return ret;
}

static int performPost(tcurl *c, char *url, char *msg) {
	clearCallbackBuffers(c);
	curl_easy_setopt( c->curlHandle, CURLOPT_POST, 1 );
	curl_easy_setopt( c->curlHandle, CURLOPT_URL, url );
        curl_easy_setopt( c->curlHandle, CURLOPT_COPYPOSTFIELDS, msg );

	int ret = CURLE_OK == curl_easy_perform(c->curlHandle);
	curl_easy_setopt( c->curlHandle, CURLOPT_POST, 0);
        curl_easy_setopt( c->curlHandle, CURLOPT_COPYPOSTFIELDS, 0 );
	return ret;

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

inline char *getLastWebResponse( tcurl *c )
{
	return c->callbackData;
}

int search( tcurl *c, char *query )
{
    int retVal = -1;
    if( isCurlInit(c) )
    {
	    strcpy(c->url, "http://search.twitter.com/search.atom?q=");
	    strcat(c->url, query);
	    retVal = performGet( c );
    }
    return retVal;
}

// Get the latest tweet along with when it was published.  We assume for the authenticated user.
int getLatestTweet(tcurl *c, char *content, int clen, long long *id)
{
        *id = 0;
	strcpy(c->url, "http://twitter.com/statuses/user_timeline/");
	strcat(c->url, c->user);
	strcat(c->url, ".xml?count=1");
	if(! performGet(c)) return;
	char *result = getLastWebResponse(c);

	if(g_debuglevel > 7) {
	  printf("XML response from TWITTER\n");
	  printf(result);
	}

	xmlDoc *doc = xmlReadMemory(result, strlen(result), "noname.xml", NULL, 0);
	if (doc == NULL) {
	  if(g_debuglevel) printf("Failed to parse document.\n");
	  return -1;
	}
	xmlNode *node = xmlDocGetRootElement(doc);
	
	// We could use XPATH but what we want isn't that deep...

	// Locate the entry node.
	xmlNode *entry = NULL;
	for(node = node->children; node; node = node->next) {
		if(strcmp(node->name,"status") == 0) {
			entry = node;
			break;
		}
	}

	if(entry) {
		for(node = node->children; node; node = node->next) {
			if(node->type == XML_ELEMENT_NODE) {
				
				if(strcmp(node->name,"id") == 0) {
				  *id = atoll( xmlNodeGetContent(node) );
				}
				if(strcmp(node->name,"text") == 0) {
					strlcpy(content, xmlNodeGetContent(node), clen);
				}
			}
		}
		if(g_debuglevel) printf("Tweet: %lld %s\n", *id, content);
	}
	xmlFreeDoc(doc);

	return 0;
}

int sendTweet(tcurl *c, char *tweet) {
    int retVal = -1;
    if( isCurlInit(c) )
    {
      char msg[140+8];
      strcpy(msg, "status=");
      strlcat(msg, tweet, sizeof(msg));
      retVal = performPost( c, "http://twitter.com/statuses/update.xml", msg );
    }
    return retVal;
  
}
