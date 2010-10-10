/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 */
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "log.h"
#include "pachube.h"
#include "mem.h"

static const char *eeml_head = "<eeml xmlns=\"http://www.eeml.org/xsd/005\""
	 "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
	 "xsi:schemaLocation=\"http://www.eeml.org/xsd/005 http://www.eeml.org/xsd/005/005.xsd\" version=\"5\">"
	 "<environment>";
static const char *eeml_foot ="</environment></eeml>";

pach_t pach_new(char *apikey, int feedid) {
     pach_t p;

     p = mem_malloc(sizeof(pach_t), M_ZERO);
     p->api = mem_strdup(apikey, M_NONE);
     p->feedid = feedid;
     return p;
}

static int postPachube(pach_t p, char *eeml) {
	 CURL *curl;
	 CURLcode res;
	 long code;
	 curl = curl_easy_init();
	 if(curl) {
	      struct curl_slist *chunk = NULL;
	      char buff[256];

	      strlcpy(buff,"X-PachubeApiKey: ",sizeof(buff));
	      strlcat(buff, p->api, sizeof(buff));
	      chunk = curl_slist_append(chunk, buff);
	      chunk = curl_slist_append(chunk, "Expect:"); // disable this.
	      sprintf(buff,"Content-Length: %d", strlen(eeml));
	      chunk = curl_slist_append(chunk, buff);

	      char url[100];
	      sprintf(url,"http://www.pachube.com/api/%d.xml?_method=put", p->feedid);
	      curl_easy_setopt(curl, CURLOPT_URL, url);

	      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, eeml);
	      curl_easy_setopt(curl, CURLOPT_POST, 1L);
		 if(getLoglevel() == LOG_DEBUG)
		   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	      res = curl_easy_perform(curl);
	      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	      curl_easy_cleanup(curl);

	      if(code != 200) {
		   code = -code;
	      }
	      return code;
	 }
	 return -1;
}

/* Send a pre-formated XML datastream set to pachube */
int pach_updateDatastreamXml(pach_t p, char *xml) {
     if(xml == NULL) return;

     info("feed %d", p->feedid);

     char *eeml = mem_malloc(strlen(eeml_head) + strlen(eeml_foot) + strlen(xml) + 1, M_NONE);
     strcpy(eeml, eeml_head);
     strcat(eeml, xml);
     strcat(eeml, eeml_foot);
     int ret = postPachube(p, eeml);
     mem_free(eeml);
     return ret;
}

void pach_destroy(pach_t p) {
     if(p->api) mem_free(p->api);
     mem_free(p);
}
