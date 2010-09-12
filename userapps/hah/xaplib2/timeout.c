/* $Id$
*/
#include <stdlib.h>
#include <time.h>
#include "xap.h"

void xapAddTimeoutAction(xAP *this, void (*func)(xAP *, int, void *), int interval, void *data) {
	xAPTimeoutCallback *cb = (xAPTimeoutCallback *)malloc(sizeof(xAPTimeoutCallback));
	cb->callback = func;
	cb->user_data = data;
	cb->interval = interval;
	cb->ttl = 0;

	cb->next = this->timeoutList;
	this->timeoutList = cb;
}

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

#ifdef TESTING
// cc -DTESTING -o timeout timeout.c
#include <stdio.h>
#include <string.h>
void cb(xAP *this, int interval, void *data) {
	char *s = (char *)data;
	time_t now = time(NULL);
	char *t = ctime(&now);
	t[strlen(t)-1] = 0;
	printf("%s - Interval %d - %s\n", t, interval, s);
}

int main(int argc, char *argv[]) {
	xAP *me = (xAP *)calloc(sizeof(xAP), 1);
	xapAddTimeoutAction(me, &cb, 5, "hello");
	xapAddTimeoutAction(me, &cb, 7, "there");
	int i;
	for(i=0; i<20; i++) {
		timeoutDispatch(me);
		sleep(1);
	}
}
#endif

