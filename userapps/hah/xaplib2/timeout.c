/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdlib.h>
#include <time.h>
#include "xap.h"

/** Add a timeout callback to the list
*/
void xapAddTimeoutAction(xAP *this, void (*func)(xAP *, int, void *), int interval, void *data) {
	xAPTimeoutCallback *cb = (xAPTimeoutCallback *)malloc(sizeof(xAPTimeoutCallback));
	debug("Add timeout. interval=%d",interval);
	cb->callback = func;
	cb->user_data = data;
	cb->interval = interval;
	cb->ttl = 0;

	cb->next = this->timeoutList;
	this->timeoutList = cb;
}

/** Dispatch timeout callback that have expired.
*/
void timeoutDispatch(xAP *this) {
	xAPTimeoutCallback *cb;
	time_t now = time(NULL);
	for(cb = this->timeoutList; cb; cb = cb->next) {
		if(cb->ttl <= now) { // Has the Callbacks timer expired?
			cb->ttl = now + cb->interval;
			(*cb->callback)(this, cb->interval, cb->user_data); // Dispatch it..
		}
	}
}

