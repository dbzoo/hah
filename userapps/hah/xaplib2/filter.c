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

xAPFilter *xapAddFilter(xAPFilter **f, char *section, char *key, char *value) {
	debug("section=%s key=%s value=%s", section, key, value);
	xAPFilter *e = (xAPFilter *)malloc(sizeof(xAPFilter));
	e->section = section;
	e->key = key;
	e->value = value;

	LL_PREPEND(*f, e);
	return e;
}

// Match a filterAddr address against a sub address
int xapFilterAddrSubaddress(char *filterAddr, char *addr) {
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
				match &= tolower(*filterAddr) == tolower(*addr);
			}
			filterAddr++;
			addr++;
		}
	}
	return match;
}

// Test if a set of filters match the xap message
// returns: 1 if a match was found, otherwise 0
int xapCompareFilters(xAPFilter *head) {
	int match = 1;
	xAPFilter *f = head;
	while(f && match) {
		char *value = xapGetValue(f->section, f->key);
		if(value == NULL) { // No section key.  We can't match.
			match = 0;
			break;
		}
		match &= xapFilterAddrSubaddress(value, f->value);
		f = f->next;
	}
	debug("match=%d",match);
	return match;
}

xAPFilterCallback *xapAddFilterAction(void (*func)(void *), xAPFilter *filter, void *data) {
	if(filter)  // a NULL filter will match everything.
		debug("Add filter. section=%s key=%s value=%s", filter->section, filter->key, filter->value);
	xAPFilterCallback *cb = (xAPFilterCallback *)malloc(sizeof(xAPFilterCallback));
	cb->callback = func;
	cb->user_data = data;
	cb->filter = filter;

	LL_PREPEND(gXAP->filterList, cb);
	return cb;
}

void filterDispatch() {
	xAPFilterCallback *cb;
	LL_FOREACH(gXAP->filterList, cb) {
		// Do all the filters for this Callback match?
		if(xapCompareFilters(cb->filter)) {
			(*cb->callback)(cb->user_data); // Dispatch it..
		}
	}
}
