/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xap.h"

xAP *gXAP;

int main(int argc, char *argv[]) {
	char *msg="xap-header\n"
	"{\n"
	"v=12\n"
	"hop=1\n"
	"uid=FF776107\n"
	"class=xAPBSC.event\n"
	"source= ACME.Lighting.apartment:BedsideLamp\n"
	"}\n"
	"input.state\n"
	"{\n"
	"state=ON\n"
	"Level= 64/255\n"
	"}\n";

	gXAP = (xAP *)calloc(sizeof(xAP), 1);

	strcpy((char *)gXAP->dataPacket, msg);
	gXAP->parsedMsgCount = parseMsg(gXAP->parsedMsg, XAP_MSG_ELEMENTS, gXAP->dataPacket);
	
	char newmsg[XAP_DATA_LEN];
	parsedMsgToRaw(gXAP->parsedMsg, gXAP->parsedMsgCount, newmsg, sizeof(newmsg));
	printf("%s\n", newmsg);

	printf("state is %s\n", xapGetValue("input.state","state"));
	int state = xapIsValue("input.state","state","on");
	printf("Is state on? %s\n", (state ? "YES" : "NO"));

	char *types[] = {"unknown", "heartbeat", "ordinary"};
	printf("Message type %s\n", types[xapGetType()]);
	return 0;
}

