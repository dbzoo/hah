/* $Id$
   Copyright (c) Brett England, 2010
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xap.h"

// As filters are PREPENDED to the list the *most* significant filter, that
// is the one that will cull the most messages, should be added last to
// minimize processing time for a rejecion.
//
// Add in the order: Section key, Class key, Target key
const char *XAP_FILTER_ANY = "XAP_FILTER_ANY";
const char *XAP_FILTER_ABSENT = "XAP_FILTER_ABSENT";

xAPFilter *xapAddFilter(xAPFilter **f, char *section, char *key, char *value)
{
	info("section=%s key=%s value=%s", section, key, value);
        xAPFilter *e = (xAPFilter *)malloc(sizeof(xAPFilter));
	e->section = strdup(section);
	e->key = strdup(key);
	e->value = strdup(value);

        LL_PREPEND(*f, e);
        return e;
}

void xapFreeFilterList(xAPFilter *head) {
	xAPFilter *f, *tmp;
	LL_FOREACH_SAFE(head, f, tmp) {
		LL_DELETE(head, f);
		free(f->section);
		free(f->key);
		free(f->value);
		free(f);
	}
}

// Match a filterAddr address against a sub address
int xapFilterAddrSubaddress(char *filterAddr, char *addr)
{
        debug("filterAddr=%s addr=%s", filterAddr, addr);
        int match = 1;

        if(filterAddr == NULL || *filterAddr == '\0') {
                // empty filterAddr matches all
                match = 1;
        } else if(strchr(filterAddr,'*') == NULL && strchr(filterAddr,'>') == NULL) {
                // no wildcards - simple compare
                match = strcasecmp(filterAddr, addr) == 0 ? 1 : 0;
        } else {
                // match using wildcard logic
                while(*filterAddr && *addr && match) {
                        switch(*filterAddr) {
                        case '>':  // FilterAddr matches rest of addr...
                                return 1;
                        case '*': // skip a field in the addr
                                while(*addr && *addr != '.') {
                                        addr++;
                                }
                                filterAddr++;
                                break;
                        default:
                                match = tolower(*filterAddr) == tolower(*addr);
                        }
			if(*filterAddr) filterAddr++;
                        if(*addr) addr++;
                }
        }
        return match;
}

// Test if a set of filters match the xap message
// returns: 1 if a match was found, otherwise 0
int xapCompareFilters(xAPFilter *head)
{
        int match = 1;
        xAPFilter *f;
        for(f=head; f && match; f=f->next) {
	        char *value = xapGetValue((char *)f->section, (char *)f->key);

                if(f->value == XAP_FILTER_ABSENT) {
                        match = value == NULL ? 1 : 0;
                        continue;
                }

                if(f->value == XAP_FILTER_ANY) {
                        match = value ? 1 : 0;
                        continue;
                }

                // No section key and we need something to strcmp()
                if(value == NULL) {
                        match = 0;
                        break;
                }

                if(strcasecmp("xap-header", f->section) == 0 &&
		   (strcasecmp("target", f->key) == 0 || strcasecmp("source",f->key) == 0)) {
			if(strcasecmp("target", f->key) == 0) {
				// for target the WILD CARD will be in inbound xAP message
				match = xapFilterAddrSubaddress(value, (char *)f->value);
			} else if (strcasecmp("source", f->key) == 0) {
				// for source the WILD CARD will be in the FILTER itself.
				match = xapFilterAddrSubaddress((char *)f->value, value);
			}
                } else {
                        match = strcasecmp(value, f->value) == 0 ? 1 : 0;
                }
        }
        debug("match=%d",match);
        return match;
}

xAPFilterCallback *xapAddFilterAction(void (*func)(void *), xAPFilter *filter, void *data)
{
        if(filter)  { // a NULL filter will match everything.
                info("section=%s key=%s value=%s data=%x", filter->section, filter->key, filter->value, data);
        }
        xAPFilterCallback *cb = (xAPFilterCallback *)malloc(sizeof(xAPFilterCallback));
        cb->callback = func;
        cb->user_data = data;
        cb->filter = filter;

        LL_PREPEND(gXAP->filterList, cb);
        return cb;
}

void filterDispatch()
{
        xAPFilterCallback *cb;
        LL_FOREACH(gXAP->filterList, cb) {
                // Do all the filters for this Callback match?
                if(xapCompareFilters(cb->filter)) {
                        (*cb->callback)(cb->user_data); // Dispatch it..
                }
        }
}
