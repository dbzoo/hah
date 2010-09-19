/* $Id: timeout.c 78 2010-09-12 23:01:29Z dbzoo.com $
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

int main(int argc, char *argv[]) {
	xAP *me = (xAP *)calloc(sizeof(xAP), 1);
	xapAddTimeoutAction(me, &cb, 5, "hello");
	xapAddTimeoutAction(me, &cb, 7, "there");
	int i;
	for(i=0; i<20; i++) {
		timeoutDispatch(me);
		sleep(1);
	}
	return 0;
}
