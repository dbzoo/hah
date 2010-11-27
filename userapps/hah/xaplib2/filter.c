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

//
// Add in the order: Section key, Class key, Target key
const char *XAP_FILTER_ANY = "XAP_FILTER_ANY";
const char *XAP_FILTER_ABSENT = "XAP_FILTER_ABSENT";

/** Add a new filter pattern to a chain of filters.
 * As filters are PREPENDED to the list the *most* significant filter, that
 * is the one that will cull the most messages, should be added last to
 * minimize processing time for a rejecion.
 * 
 * @param f Head of the filter chain
 * @param section xAP section
 * @param key  xAP key
 * @param value  xAP value
 * @return Pointer to newly added filter
 */
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


/** Match a filterAddr address against a sub address
 * 
 * @param filterAddr A pattern optionally containing wildcards to match against
 * @param addr An xAP address
 * @return 1 if a match was found, otherwise 0
 */
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

/** Test if a set of filters match the xap message
 * 
 * @param head The start of a filter chain
 * @return 1 if a match was found, otherwise 0
 */
int xapCompareFilters(xAPFilter *head)
{
        int match = 1;
        xAPFilter *f;
        for(f=head; f && match; f=f->next) {
	        char *value = xapGetValue((char *)f->section, (char *)f->key);

                if(strcmp(f->value,XAP_FILTER_ABSENT) == 0) {
                        match = value == NULL ? 1 : 0;
                        continue;
                }

                if(strcmp(f->value, XAP_FILTER_ANY) == 0) {
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

/** Free a chain of filter patterns
 * 
 * @param head The start of a filter chain
 */
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

/** Delete a filter callback
 *
 * @param f xAPFilterCallback to delete
 * @return pointer to user data, caller is responsible for this memory.
 */
void *xapDelFilterAction(xAPFilterCallback *f) {
	void *userData = f->user_data;
	LL_DELETE(gXAP->filterList, f);
	xapFreeFilterList(f->filter);
	free(f);
	return userData;
}

/** Add a filter action
 * 
 * @param (* func)( void * ) User callback function, User data pointer will handed back.
 * @param filter The head of a link list of filter patterns
 * @param data User supplied callback data.
 * @return The filter callback created and added.
 */
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

/**
 * Walk the list of filter actions when all the filters patterns match
 * call the associated callback function.
 */
void filterDispatch()
{
        xAPFilterCallback *cb, *tmp;
        LL_FOREACH_SAFE(gXAP->filterList, cb, tmp) {
                // Do all the filters for this Callback match?
                if(xapCompareFilters(cb->filter)) {
                        (*cb->callback)(cb->user_data); // Dispatch it..
                }
        }
}
