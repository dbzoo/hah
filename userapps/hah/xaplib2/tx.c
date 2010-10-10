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
#include <sys/types.h>
#include <sys/socket.h>
#include "xap.h"

void xapSend(const char *mess)
{
	info("send\n%s", mess);
	if(sendto(gXAP->txSockfd, mess, strlen(mess), 0, (struct sockaddr *)&(gXAP->txAddress), sizeof(struct sockaddr_in)) < 0)
		err_strerror("Tx xAP packet");
}

/** Fills out a short xap message with missing information.
*
* If UID or SOURCE is supplied they will be replaced with those from the params.
*
* xap-header
* {
* target=
* class=
* }
*
* Will become
*
* xap-header
* {
* v=12
* uid=
* hop=1
* source=
* target=
* class=
* }
*/
char *fillShortXap(char *shortMsg, char *uid, char *source) {
	strlcpy((char *)gXAP->dataPacket, shortMsg, XAP_DATA_LEN);
	parseMsg();

	char *target = xapGetValue("xap-header", "target");
	char *class = xapGetValue("xap-header", "class");
	if(target == NULL) {
		err("Missing mandatory target element");
		return NULL;
	}
	if(class == NULL) {
		err("Missing mandatory class element");
		return NULL;
	}

	// Copy these out as the PARSE buffer is going to be reused.
	char *starget = strdup(target);
	char *sclass = strdup(class);

	char xapBody[XAP_DATA_LEN];
	parsedMsgToRawWithoutSection(xapBody, sizeof(xapBody), "xap-header");
	
	char newMsg[XAP_DATA_LEN];
	snprintf(newMsg, XAP_DATA_LEN, "xap-header\n"
	               "{\n"
	               "v=12\n"
	               "hop=1\n"
	               "uid=%s\n"
	               "class=%s\n"
	               "source=%s\n"
	               "target=%s\n"
	               "}\n"
	               "%s",
	               uid, sclass, source, starget, xapBody);
	
	free(sclass);
	free(starget);
	return strdup(newMsg);
}
