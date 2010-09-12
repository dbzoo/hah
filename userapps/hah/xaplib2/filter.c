/* $Id$
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

void xapAddFilter(xAPFilter **f, char *section, char *key, char *value) {
	xAPFilter *e = (xAPFilter *)malloc(sizeof(xAPFilter));
	e->section = section;
	e->key = key;
	e->value = value;

	// PREPEND to list
	e->next = *f;
	*f = e;
}

// Match a filterAddr address against a sub address
int xapFilterAddrSubaddress(char *filterAddr, char *addr) {
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
int xapCompareFilters(xAP *this, xAPFilter *head) {
	int match = 1;
	xAPFilter *f = head;
	while(f && match) {
		char *value = xapGetValue(this, f->section, f->key);
		if(value == NULL) { // No section key.  We can't match.
			match = 0;
			break;
		}
		match &= xapFilterAddrSubaddress(value, f->value);
		f = f->next;
	}
	return match;
}

void xapAddFilterAction(xAP *this, void (*func)(xAP *, void *), xAPFilter *filter, void *data) {
	xAPFilterCallback *cb = (xAPFilterCallback *)malloc(sizeof(xAPFilterCallback));
	cb->callback = func;
	cb->user_data = data;
	cb->filter = filter;

	cb->next = this->filterList;
	this->filterList = cb;
}

void filterDispatch(xAP *this) {
	xAPFilterCallback *cb;
	for(cb = this->filterList; cb; cb = cb->next) {
		// Do all the filters for this Callback match?
		if(xapCompareFilters(this, cb->filter)) {
			(*cb->callback)(this, cb->user_data); // Dispatch it..
		}
	}
}

#ifdef TESTING2
// cc -DTESTING2 -o filter filter.c
#include "parse.c"
int main(int argc, char **argv) {
	xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.1.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.1.>");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.*.Controller");
	xapAddFilter(&f, "xap-header","target","dbzoo.livebox.>");

	xAPFilter *e;
	for(e = f; e; e = e->next) {
		int match = xapFilterAddrSubaddress(e->value, "dbzoo.livebox.1.controller");
		printf("%s { %s=%s } - %d\n", e->section, e->key, e->value, match);
	}
}
#endif
