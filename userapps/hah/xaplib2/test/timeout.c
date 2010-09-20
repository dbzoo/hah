/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "xap.h"

void cb(xAP *this, int interval, void *data) {
	char *s = (char *)data;
	time_t now = time(NULL);
	char *t = ctime(&now);
	t[strlen(t)-1] = 0;
	printf("%s - Interval %d - %s\n", t, interval, s);
}

void hello(xAP *this, int interval, void *data) {
	static int i = 0;
	cb(this, interval, data);
	i++;
	if(i == 2) { // After being invoked 2 times speed up.
		xapDelTimeoutActionByFunc(this, &hello);
		interval -= 2; // Make it 2 seconds quicker.
		printf("removing Hello timeout - Resubmitting every %d sec\n", interval);
		xapAddTimeoutAction(this, &cb, interval, "hello");
	}
}

int main(int argc, char *argv[]) {
	xAP *me = (xAP *)calloc(sizeof(xAP), 1);
	xapAddTimeoutAction(me, &hello, 5, "hello");
	xapAddTimeoutAction(me, &cb, 7, "there");
	int i;
	for(i=0; i<20; i++) {
		timeoutDispatch(me);
		sleep(1);
	}
	return 0;
}
