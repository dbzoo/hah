/* Copyright (c) Brett England, 2010
 
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
xAPTimeoutCallback *xapAddTimeoutAction(void (*func)(int, void *), int interval, void *data)
{
	die_if(interval < 1, "Invalid timeout interval %d secs", interval);	
	debug("Add timeout. interval=%d",interval);
	xAPTimeoutCallback *cb = (xAPTimeoutCallback *)malloc(sizeof(xAPTimeoutCallback));
        cb->callback = func;
        cb->user_data = data;
        cb->interval = interval;
        cb->ttl = 0;

        LL_PREPEND(gXAP->timeoutList, cb);
        return cb;
}

void xapTimeoutReset(xAPTimeoutCallback *cb) {
	cb->ttl = time(NULL) + cb->interval;
}

void xapTimeoutExpire(xAPTimeoutCallback *cb) {
        cb->ttl = 0;
}

/** Remove a timeout and free the memory for the callback.
*/
void *xapDelTimeoutAction(xAPTimeoutCallback *cb)
{
	void *userData = cb->user_data;
        LL_DELETE(gXAP->timeoutList, cb);
        free(cb);
	return userData;
}

xAPTimeoutCallback *xapFindTimeoutByFunc(void (*func)(int, void*)) {
        xAPTimeoutCallback *e;
	LL_SEARCH_SCALAR(gXAP->timeoutList, e, callback, func);
	return e;
}

xAPTimeoutCallback *xapFindTimeoutByUserData(void *userData) {
        xAPTimeoutCallback *e;
	LL_SEARCH_SCALAR(gXAP->timeoutList, e, user_data, userData);
	return e;
}

/** Remove a timeout using the callback func as a lookup key.
* If multiple timeouts are registered for the same callback func
* all will be deleted.
*/
void *xapDelTimeoutActionByFunc(void (*func)(int, void *))
{
        xAPTimeoutCallback *e, *tmp;
        LL_FOREACH_SAFE(gXAP->timeoutList, e, tmp) {
                if(e->callback == func)
                        return xapDelTimeoutAction(e);
        }
	return NULL;
}

/** Dispatch timeout callback that have expired.
*/
void timeoutDispatch()
{
        xAPTimeoutCallback *cb;
        time_t now = time(NULL);
        LL_FOREACH(gXAP->timeoutList, cb) {
                if(cb->ttl <= now) { // Has the Callbacks timer expired?
                        cb->ttl = now + cb->interval;
                        (*cb->callback)(cb->interval, cb->user_data); // Dispatch it..
                }
        }
}

