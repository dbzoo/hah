/* $Id$
 *
 * XAP Message cache
 */
#include <stdio.h>
#include <string.h>
#include "msg_cache.h"

static xapEntry msgCache[CACHE_SIZE];
static int cache_size = 0;

// Locate a message in the CACHE.
xapEntry *findMessage(const char *source, const char *target, const char *class) {
     if(g_debuglevel > 7) printf("findMessage src-%s tgt-%s cls-%s\n", source, target, class);
     int i;
     for(i=0; i<cache_size; i++)
     {
	  if((target == NULL || *target == '\0' || strcasecmp(msgCache[i].target, target) == 0) &&
	     (source == NULL || *source == '\0' || strcasecmp(msgCache[i].source, source) == 0) &&
	     strcasecmp(msgCache[i].class, class) == 0) 
	  {
	       return &msgCache[i];
	  }
     }
     return NULL;
}

// Add/Update a message to the CACHE
xapEntry *addMessage(const char *msg) {
     char target[XAP_MAX_KEYVALUE_LEN];
     char source[XAP_MAX_KEYVALUE_LEN];
     char class[XAP_MAX_KEYVALUE_LEN];

     int elements = xapmsg_parse(msg);

     if(xapmsg_gettype() != XAP_MSG_ORDINARY) return NULL; 
     if(xapmsg_getvalue("xap-header:source", source) == 0 ) return NULL; // Mandatory
     if(xapmsg_getvalue("xap-header:class", class) == 0 ) return NULL;  // Mandatory
     if(xapmsg_getvalue("xap-header:target", target) == 0) { // Optional
	  target[0] = '\0';
     }

     xapEntry *entry = findMessage(source,target,class);
     if(entry == NULL) {
	  if(g_debuglevel > 7) printf("Adding Cache entry %d\n", cache_size);
	  entry = &msgCache[cache_size];
	  // Out of cache?  Just reuse the last entry over and over.
	  if(cache_size < CACHE_SIZE-1) {
	       cache_size++;
	  }
	  strlcpy(entry->target, target, sizeof(entry->target));
	  strlcpy(entry->source, source, sizeof(entry->source));
	  strlcpy(entry->class, class, sizeof(entry->class));
     }
     entry->loaded = time(NULL);
     entry->elements = 0;

     // Cache remainder of message minus the header which is stored separately.
     int i;
     for(i=0; i<elements && entry->elements < MAX_ELEMENTS; i++) {
	  struct tg_xap_msg *map = &g_xap_msg[i];
	  if(strcasecmp(map->section,"xap-header") == 0) continue;
	  strcpy(entry->msg[entry->elements].section, map->section);
	  strcpy(entry->msg[entry->elements].name, map->name);
	  strcpy(entry->msg[entry->elements].value, map->value);
	  entry->elements++;
     }

     return entry;
}

void dump_cacheEntry(xapEntry *entry) {
     printf("Dump cache entry\n", entry);
     if(entry == NULL) return;
     printf("\tsource: %s\n", entry->source);
     printf("\ttarget: %s\n", entry->target);
     printf("\tclass: %s\n", entry->class);
     int i;
     for(i=0; i<entry->elements;i++) {
	  printf("\t%s:%s=%s\n", entry->msg[i].section, entry->msg[i].name, entry->msg[i].value);
     }
}
