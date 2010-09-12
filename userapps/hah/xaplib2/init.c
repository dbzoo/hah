/* $Id$
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "xap.h"

// Setup for Rx XAP packets
int discoverHub(int *rxport, int *rxfd, struct sockaddr_in *txAddr)
{
        struct sockaddr_in s;

        *rxfd = socket(AF_INET, SOCK_DGRAM, 0); // Non-blocking listener
	*rxport = XAP_PORT_L;
	fcntl(*rxfd, F_SETFL, O_NONBLOCK);

        s.sin_family = AF_INET;
        s.sin_port = htons(*rxport);
	s.sin_addr.s_addr = txAddr->sin_addr.s_addr;  // broadcast addr

        if (bind(*rxfd, (struct sockaddr *)&s, sizeof(s)) == -1) {
                printf("Broadcast socket port %d in use\n", XAP_PORT_L);
                printf("Assuming a hub is active\n");
                int i;
                for (i=XAP_PORT_L+1; i < XAP_PORT_H; i++)
                {
                        s.sin_family = AF_INET;
                        s.sin_addr.s_addr = inet_addr("127.0.0.1");
                        s.sin_port = htons(i);

                        if (bind(*rxfd, (struct sockaddr *)&s, sizeof(struct sockaddr)) == -1) {
                                printf("Socket port %d in use\n", i);
                        } else {
                                printf("Discovered port %d\n", i);
                                *rxport = i;
                                break;
                        }
                }
        } else {
                printf("Acquired broadcast socket, port %d\n", *rxport);
                printf("Assuming no local hub is active\n");
        }
	return 1;
}

// Setup for Tx XAP Packets
int discoverBroadcastNetwork(struct sockaddr_in *txAddr, int *txfd, char *interfaceName)
{
        struct ifreq interface;
        struct sockaddr_in myinterface;
        struct sockaddr_in mynetmask;
        int optval, optlen;

        if (interfaceName == NULL)
                interfaceName = "eth0";

        // Discover the broadcast network settings
        *txfd = socket(AF_INET, SOCK_DGRAM, 0);

        optval=1;
        optlen=sizeof(int);
        if (setsockopt(*txfd, SOL_SOCKET, SO_BROADCAST, (char*)&optval, optlen))
        {
	        perror("discoverBroadcastNetwork");
                printf("Cannot set options on broadcast socket\n");
                return -1;
        }

        // Query the low-level capabilities of the network interface
        // we are to use. If none passed on command line, default to
        // eth0.
        memset((char*)&interface, sizeof(interface),0);
        interface.ifr_addr.sa_family = AF_INET;
        strcpy(interface.ifr_name, interfaceName);
        if (ioctl(*txfd, SIOCGIFADDR, &interface))
        {
	        perror("discoverBroadcastNetwork");
	        printf("Could not determine interface address for interface %s\n", interfaceName);
                return -1;
        }
        myinterface.sin_addr.s_addr=((struct sockaddr_in*)&interface.ifr_broadaddr)->sin_addr.s_addr;

	printf("%s: address %s\n", interfaceName, inet_ntoa(((struct sockaddr_in *)&interface.ifr_addr)->sin_addr));
	
        // Find netmask
        interface.ifr_addr.sa_family = AF_INET;
        interface.ifr_broadaddr.sa_family = AF_INET;
        strcpy(interface.ifr_name, interfaceName);
        if (ioctl(*txfd, SIOCGIFNETMASK, &interface))
        {
	        perror("discoverBroadcastNetwork");
                printf("Unable to determine netmask for interface %s\n", interfaceName);
                return -1;
        }
        mynetmask.sin_addr.s_addr = ((struct sockaddr_in*)&interface.ifr_broadaddr)->sin_addr.s_addr;

	printf("%s: netmask %s\n", interfaceName, inet_ntoa(((struct sockaddr_in *)&interface.ifr_netmask)->sin_addr));
	
        // Calculate broadcast address and stuff TX_address struct
        txAddr->sin_addr.s_addr = ~mynetmask.sin_addr.s_addr | myinterface.sin_addr.s_addr;
        txAddr->sin_family = AF_INET;
        txAddr->sin_port = htons(XAP_PORT_L);

        printf("Autoconfig: xAP broadcasts on %s:%d\n",inet_ntoa(txAddr->sin_addr), XAP_PORT_L);

        return 0;
}

void heartbeatHandler(xAP *this, int interval, void *data)
{
        char buff[XAP_DATA_LEN];
        sprintf(buff, "xap-hbeat\n"
                "{\n"
                "v=12\n"
                "hop=1\n"
                "uid=%s\n"
                "class=xap-hbeat.alive\n"
                "source=%s\n"
                "interval=%d\n"
                "port=%d\n"
                "}\n", this->uid, this->source, interval, this->rxPort);
        xapSend(this, buff);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void xapAddSocketListener(xAP *xap, int fd, void (*callback)(xAP *, int, void *), void *data)
{
        xAPSocketConnection *cb = (xAPSocketConnection *)malloc(sizeof(xAPSocketConnection));
        cb->callback = callback;
        cb->user_data = data;
        cb->fd = fd;

        cb->next = xap->connectionList;
        xap->connectionList = cb;
}

xAP *xapNew(char *source, char *uid, char *interfaceName)
{
	assert(uid && strlen(uid) == 8);

	xAP *self = (xAP *)calloc(sizeof(xAP), 1);

	if(discoverBroadcastNetwork(&self->txAddress, &self->txSockfd, interfaceName) == -1)
		goto error;
	if(discoverHub(&self->rxPort, &self->rxSockfd, &self->txAddress) == -1)
		goto error;
	
	self->source = strdup(source);
	self->uid = strdup(uid);

	xapAddTimeoutAction(self, &heartbeatHandler, XAP_HEARTBEAT_INTERVAL, NULL);
        xapAddSocketListener(self, self->rxSockfd, handleXapPacket, NULL);

        return self;
error:
	free(self);
	return NULL;
}
