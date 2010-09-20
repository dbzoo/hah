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

/** Add a timeout callback to the list.
*/
xAPTimeoutCallback *xapAddTimeoutAction(xAP *xap, void (*func)(xAP *, int, void *), int interval, void *data)
{
	die_if(interval < 1, "Invalid timeout interval %d secs", interval);	
	debug("Add timeout. interval=%d",interval);
	xAPTimeoutCallback *cb = (xAPTimeoutCallback *)malloc(sizeof(xAPTimeoutCallback));
        cb->callback = func;
        cb->user_data = data;
        cb->interval = interval;
        cb->ttl = 0;

        LL_PREPEND(xap->timeoutList, cb);
        return cb;
}

/** Remove a timeout and free the memory for the callback.
*/
void xapDelTimeoutAction(xAP *xap, xAPTimeoutCallback **cb)
{
        LL_DELETE(xap->timeoutList, *cb);
        free(*cb);
	*cb = NULL;
}

/** Remove a timeout using the callback func as a lookup key.
* If multiple timeouts are registered for the same callback func
* ALL will be deleted.
*/
void xapDelTimeoutActionByFunc(xAP *xap, void (*func)(xAP *, int, void *))
{
        xAPTimeoutCallback *e, *tmp;
        LL_FOREACH_SAFE(xap->timeoutList, e, tmp) {
                if(e->callback == func)
                        xapDelTimeoutAction(xap, &e);
        }
}

/** Dispatch timeout callback that have expired.
*/
void timeoutDispatch(xAP *this)
{
        xAPTimeoutCallback *cb;
        time_t now = time(NULL);
        LL_FOREACH(this->timeoutList, cb) {
                if(cb->ttl <= now) { // Has the Callbacks timer expired?
                        cb->ttl = now + cb->interval;
                        (*cb->callback)(this, cb->interval, cb->user_data); // Dispatch it..
                }
        }
}

