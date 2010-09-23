/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
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
