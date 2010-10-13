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
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "minIni.h"
#include "xap.h"

xAP *gXAP;  // Our global XAP object

inline char *xapGetSource() {
	return gXAP->source;
}

inline char *xapGetUID() {
	return gXAP->uid;
}

inline char *xapGetIP() {
	return gXAP->ip;
}

/// Setup for Rx XAP packets
void discoverHub(int *rxport, int *rxfd, struct sockaddr_in *txAddr)
{
        struct sockaddr_in s;

        *rxfd = socket(AF_INET, SOCK_DGRAM, 0); // Non-blocking listener
        *rxport = XAP_PORT_L;
        fcntl(*rxfd, F_SETFL, O_NONBLOCK);

        s.sin_family = AF_INET;
        s.sin_port = htons(*rxport);
        s.sin_addr.s_addr = txAddr->sin_addr.s_addr;  // broadcast addr

        if (bind(*rxfd, (struct sockaddr *)&s, sizeof(s)) == -1)
        {
                notice("Broadcast socket port %d in use", XAP_PORT_L);
                info("Assuming a hub is active");
                int i;
                for (i=XAP_PORT_L+1; i < XAP_PORT_H; i++)
                {
                        s.sin_family = AF_INET;
                        s.sin_addr.s_addr = inet_addr("127.0.0.1");
                        s.sin_port = htons(i);

                        if (bind(*rxfd, (struct sockaddr *)&s, sizeof(struct sockaddr)) == -1) {
                                notice("Socket port %d in use", i);
                        } else {
                                info("Discovered port %d", i);
                                *rxport = i;
                                break;
                        }
                }
                die_if(i > XAP_PORT_H,"Could not allocate a port from the range %d to %d", XAP_PORT_L, XAP_PORT_H);
        } else
        {
                info("Acquired broadcast socket, port %d", *rxport);
                info("Assuming no local hub is active");
        }
}

/// Setup for Tx XAP Packets
void discoverBroadcastNetwork(struct sockaddr_in *txAddr, int *txfd, char **ip, char *interfaceName)
{
        struct ifreq interface;
        struct sockaddr_in myinterface;
        struct sockaddr_in mynetmask;
        int optval, optlen;

        if (interfaceName == NULL)
        {
                interfaceName = "eth0";
                info("Defaulting interface to %s", interfaceName);
        }

        if((*txfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
                die_strerror("Unable to create Tx socket");
        }

        optval=1;
        optlen=sizeof(int);
        if (setsockopt(*txfd, SOL_SOCKET, SO_BROADCAST, (char*)&optval, optlen) == -1)
        {
                die_strerror("Cannot set options on broadcast socket");
        }

        // Query the low-level capabilities of the network interface
        // we are to use. If none passed on command line, default to
        // eth0.
        memset((char*)&interface, sizeof(interface),0);
        interface.ifr_addr.sa_family = AF_INET;
        strcpy(interface.ifr_name, interfaceName);
        if (ioctl(*txfd, SIOCGIFADDR, &interface) == -1)
        {
                die_strerror("Unable to query capabilities of interface %s", interfaceName);
        }
        myinterface.sin_addr.s_addr=((struct sockaddr_in*)&interface.ifr_broadaddr)->sin_addr.s_addr;

        *ip = strdup(inet_ntoa(((struct sockaddr_in *)&interface.ifr_addr)->sin_addr));
        info("%s: address %s", interfaceName, *ip);

        // Find netmask
        interface.ifr_addr.sa_family = AF_INET;
        interface.ifr_broadaddr.sa_family = AF_INET;
        strcpy(interface.ifr_name, interfaceName);
        if (ioctl(*txfd, SIOCGIFNETMASK, &interface) == -1)
        {
                die_strerror("Unable to determine netmask for interface %s", interfaceName);
        }
        mynetmask.sin_addr.s_addr = ((struct sockaddr_in*)&interface.ifr_broadaddr)->sin_addr.s_addr;

        info("%s: netmask %s", interfaceName, inet_ntoa(((struct sockaddr_in *)&interface.ifr_netmask)->sin_addr));

        // Calculate broadcast address and stuff TX_address struct
        txAddr->sin_addr.s_addr = ~mynetmask.sin_addr.s_addr | myinterface.sin_addr.s_addr;
        txAddr->sin_family = AF_INET;
        txAddr->sin_port = htons(XAP_PORT_L);

        info("Autoconfig: xAP broadcasts on %s:%d",inet_ntoa(txAddr->sin_addr), XAP_PORT_L);
}

/** Timeout handler to send heartbeats.
*/
void heartbeatHandler(int interval, void *data)
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
                "pid=%d\n"
                "}\n", xapGetUID(), xapGetSource(), interval, gXAP->rxPort, getpid());
        xapSend(buff);
}

////////////////////////////////////////////////////////////////////////////////////////////////

xAPSocketConnection *xapFindSocketListenerByFD(int ifd)
{
        xAPSocketConnection *e;
        LL_SEARCH_SCALAR(gXAP->connectionList, e, fd, ifd);
        return e;
}

void xapDelSocketListener(xAPSocketConnection **cb)
{
        LL_DELETE(gXAP->connectionList, *cb);
        free(*cb);
        *cb = NULL;
}

/** select() on other descriptors whilst in the xapProcess() loop.
*/
xAPSocketConnection *xapAddSocketListener(int fd, void (*callback)(int, void *), void *data)
{
        die_if(fd < 0, "Invalid socket %d", fd);
        debug("socket=%d", fd);
        xAPSocketConnection *cb = (xAPSocketConnection *)malloc(sizeof(xAPSocketConnection));
        cb->callback = callback;
        cb->user_data = data;
        cb->fd = fd;

        LL_PREPEND(gXAP->connectionList, cb);
        return cb;
}

/** Create a new xAP object.
* Creates Tx/Rx sockets.
* Registers a heartbeat timeout callback.
* Registers the Rx packet handler for xAP data.
*/
void xapInit(char *source, char *uid, char *interfaceName)
{
        die_if(uid == NULL || strlen(uid) != 8, "UID must be of length 8");

        gXAP = (xAP *)calloc(sizeof(xAP), 1);

        discoverBroadcastNetwork(&gXAP->txAddress, &gXAP->txSockfd, &gXAP->ip, interfaceName);
        discoverHub(&gXAP->rxPort, &gXAP->rxSockfd, &gXAP->txAddress);

        gXAP->source = strdup(source);
        gXAP->uid = strdup(uid);

        xapAddTimeoutAction(&heartbeatHandler, XAP_HEARTBEAT_INTERVAL, NULL);
        xapAddSocketListener(gXAP->rxSockfd, handleXapPacket, NULL);
}

/**
 * Register an xap connection initialized from an INI file.
 */
void xapInitFromINI(
        char *section, char *prefix, char *instance, char *uid,
        char *interfaceName, const char *inifile)
{
        long n;
        char i_uid[5];
        char s_uid[10];

        n = ini_gets(section,"uid",uid,i_uid, sizeof(i_uid),inifile);

        // validate that the UID can be read as HEX
        if(! (n > 0
                        && (isxdigit(i_uid[0]) && isxdigit(i_uid[1]) &&
                            isxdigit(i_uid[2]) && isxdigit(i_uid[3]))
                        && strlen(i_uid) == 4)) {
                err("invalid uid %s", i_uid);
                strcpy(i_uid,uid); // not valid put back default.
        }
        snprintf(s_uid, sizeof(s_uid), "FF%s00", i_uid);

        char i_control[64];
        char s_control[128];
        n = ini_gets("xap","instance","",i_control,sizeof(i_control),inifile);
        strcpy(s_control, prefix);
        // Make sure the prefix has a trailing DOT.
        if(prefix[strlen(prefix)-1] != '.') {
                strlcat(s_control,".", sizeof(s_control));
        }
        // If there a unique HAH sub address component?
        if(i_control[0]) {
                strlcat(s_control, i_control, sizeof(s_control));
                strlcat(s_control, ".",sizeof(s_control));
        }
        strlcat(s_control,instance,sizeof(s_control));

        xapInit(s_control, s_uid, interfaceName);
        die_if(gXAP == NULL,"Failed to init xAP");
}


/// Display usage information and exit.
static void simpleUsage(char *prog, char *interfaceName)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF     Default %s\n", interfaceName);
        printf("  -d, --debug            0-7\n");
        printf("  -h, --help\n");
        exit(1);
}

void simpleCommandLine(int argc, char *argv[], char **interfaceName)
{
        int i;
        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        *interfaceName = argv[++i];
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
	                simpleUsage(argv[0], *interfaceName);
                }
        }
}
