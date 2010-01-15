#include "xapdef.h"

#ifndef _MSG_CACHE_H
#define _MSG_CACHE_H

#define CACHE_SIZE 100

// the XAP library may parse more in a single message than our cache
// but as we are holding 100 unique messages and space is statically allocated
// so we will hold less entries.  Typically there are < 20 rows in a message.
// Also we don't cache the xap-header entries either.
#define MAX_ELEMENTS 20

typedef struct _xapCache {
     char source[64];
     char target[64];
     char class[64];
     struct tg_xap_msg msg[MAX_ELEMENTS];
     int elements;
     time_t loaded;
} xapEntry;

xapEntry *findMessage(const char *source, const char *target, const char *class);
xapEntry *addMessage(const char *msg);

#endif
