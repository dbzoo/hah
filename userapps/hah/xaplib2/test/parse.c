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
#include "log.h"

xAP *gXAP;

int main(int argc, char *argv[]) {
        setLoglevel(LOG_ERR);
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
	"Level=64/255\n"
	"}\n";

	gXAP = (xAP *)calloc(sizeof(xAP), 1);

	strcpy((char *)gXAP->frame.dataPacket, msg);
	gXAP->frame.len = strlen(gXAP->frame.dataPacket);
	parseMsg();
	
	char newmsg[XAP_DATA_LEN];
	parsedMsgToRaw(newmsg, sizeof(newmsg));
	printf("%s\n", newmsg);
	
	parsedMsgToRawWithoutSection(newmsg, sizeof(newmsg),"xap-header");
	printf("BODY ONLY\n%s\n", newmsg);

	char *smsg="xap-header\n"
		"{\n"
		"class=xAPBSC.event\n"
		"target=ACME.Lighting.apartment:BedsideLamp\n"
		"}\n"
		"input.state\n"
		"{\n"
		"State=ON\n"
		"Level=64/255\n"
		"}\n";	
	char *m = fillShortXap(smsg);
	printf("SHORT MESSAGE\n%s",m);
	
	printf("state is %s\n", xapGetValue("input.state","state"));
	int state = xapIsValue("input.state","state","on");
	printf("Is state on? %s\n", (state ? "YES" : "NO"));

	char *types[] = {"unknown", "heartbeat", "ordinary"};
	printf("Message type %s\n", types[xapGetType()]);


	printf("\nTesting empty keys and lowercasing an xap message\n");
	xAPFrame f;

	strcpy(f.dataPacket,"Section\n{\nHello=WORLD\nHi=\nWorlD= hello\nthere=\n}\n");
	f.len = strlen(f.dataPacket);
	printf("%s\n", f.dataPacket);
	
	parseMsgF(&f);
	xapLowerMessageF(&f);
	
	char output[1024];
	parsedMsgToRawF(&f, output, sizeof(output));
	printf("%s\n", output);

	return 0;       
}
