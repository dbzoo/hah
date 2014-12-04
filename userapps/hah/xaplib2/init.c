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
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
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
        s.sin_addr.s_addr = htonl(INADDR_ANY); // txAddr->sin_addr.s_addr;  // SUBNET broadcast addr

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
        struct ifconf ifc;
        struct ifreq ifr[10];
        int sd, ifc_num, i, found=0;
        struct in_addr addr, bcast;

        if((sd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
          die_strerror("Unable to create interface enumerator socket");
        }

        ifc.ifc_len = sizeof(ifr);
        ifc.ifc_buf = (caddr_t)ifr;

        if(ioctl(sd, SIOCGIFCONF, &ifc) == 0) {
          ifc_num = ifc.ifc_len / sizeof(struct ifreq);
          info("%d interfaces found", ifc_num);

          for(i=0; i < ifc_num; i++) {
            if(ifr[i].ifr_addr.sa_family != AF_INET) continue;
            info("%d) interface: %s", i+1, ifr[i].ifr_name);
            if(strcmp("lo", ifr[i].ifr_name) == 0) continue;
            if(interfaceName == NULL || strcmp(ifr[i].ifr_name, interfaceName) == 0) {
              // Retrieve the IP address and compute the broadcast address.
              if (ioctl(sd, SIOCGIFADDR, &ifr[i]) == 0)
                {
                  addr.s_addr = ((struct sockaddr_in *)(&ifr[i].ifr_addr))->sin_addr.s_addr;
                  info("address: %s", inet_ntoa(addr));
                } else die_strerror("Unable to retrieve IP address");

	      long n = ini_getl("network","bcast_ones",0,"/etc/xap.d/system.ini");
	      if(n == 1) {
		bcast.s_addr = 0xffffffff;
		info("broadcast: %s (override)", inet_ntoa(bcast));
	      } else if(ioctl(sd, SIOCGIFNETMASK, &ifr[i]) == 0) {
		// Compute the broadcast from our netmask.
		// On the livebox using SIOCGIFBRDADDR can return an addr that does not match the mask/ip combination.
		// esp. for the 172.16.0.0/12 network.
		bcast.s_addr = addr.s_addr | ~((struct sockaddr_in *)(&ifr[i].ifr_broadaddr))->sin_addr.s_addr;
		info("broadcast: %s", inet_ntoa(bcast));
	      } else die_strerror("Unable to retrieve broadcast address");

              found = 1;
              break;
            }
          }
        }
        else
          err_strerror("Unable to retrieve interface list");

        if(found == 0) {
          if(interfaceName)
            die("Interface %s not found", interfaceName);
          die("No interface found");
        }

	// Set the socket to be UDP Broadcast enabled.
        int optval=1;
        int optlen=sizeof(int);
        if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char*)&optval, optlen) == -1)
          {
            die_strerror("Cannot enable socket broadcast");
          }

	// Setup the TX sockaddr options.
        txAddr->sin_addr.s_addr = bcast.s_addr;
        txAddr->sin_family = AF_INET;
        txAddr->sin_port = htons(XAP_PORT_L);

	*txfd = sd; // return the socket.
	*ip = strdup(inet_ntoa(addr)); // return a string IP 
}

/** Timeout handler to send heartbeats.
 *
 * @param interval Number of seconds between invocations
 * @param data unused
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
                "pid=%s:%d\n"
                "}\n", xapGetUID(), xapGetSource(), interval, gXAP->rxPort, xapGetIP(), getpid());
        xapSend(buff);
}

// Handler for process termination - SIGINT, SIGTERM
void termHandler(int sig)
{
        char buff[XAP_DATA_LEN];
        sprintf(buff, "xap-hbeat\n"
                "{\n"
                "v=12\n"
                "hop=1\n"
                "uid=%s\n"
                "class=xap-hbeat.stop\n"
                "source=%s\n"
                "interval=%d\n"
                "port=%d\n"
                "pid=%s:%d\n"
                "}\n", xapGetUID(), xapGetSource(), XAP_HEARTBEAT_INTERVAL, gXAP->rxPort, xapGetIP(), getpid());
        xapSend(buff);
}


////////////////////////////////////////////////////////////////////////////////////////////////

/** Locate a socket listener by its file descriptor
 *
 * @param ifd File descriptor
 * @return xap Socket connection
 */
xAPSocketConnection *xapFindSocketListenerByFD(int ifd)
{
        xAPSocketConnection *e;
        LL_SEARCH_SCALAR(gXAP->connectionList, e, fd, ifd);
        return e;
}

/** Delete a socket listener
 *
 * @param cb socket connection to be deleted
 * @return Pointer to user data.  Caller is responsible for freeing.
 */
void *xapDelSocketListener(xAPSocketConnection *cb)
{
        void *userData = cb->user_data;
        LL_DELETE(gXAP->connectionList, cb);
        free(cb);
        return userData;
}

/** Monitor multiple file descriptor callback when ready.
 * select() on other descriptors whilst in the xapProcess() loop.
 *
 * @param fd File descriptor to listen on
 * @param (* callback)( int , void * )  Callback function
 * @param data Pointer to user data
 * @return Created xAP socket connection listener
 */
xAPSocketConnection *xapAddSocketListener(int fd, void (*callback)(int, void *), void *data)
{
        if(fd < 0) {
                err("Invalid socket %d", fd);
                return NULL;
        }
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

        signal(SIGTERM, termHandler);
        signal(SIGINT, termHandler);
}

/** compute a deviceid
    0 on success, otherwise -1
 */
int xapGetDeviceID(char *hostname, size_t len) {
  // If there a unique HAH sub address component?
  int n = ini_gets("xap","deviceid","",hostname,len,"/etc/xap.d/system.ini");
  if(n > 0)
    return 0;

  // Fallback to hostname
  if(gethostname(hostname, len) == 0) {
    // Don't use the FQDN.
    char *host = strchr(hostname,'.');
    if (host) {
      *host = '\0';
    }
    return 0;
  }

  warning("Failed to get hostname");
  return -1;
}

char *xapBuildAddress(char *vendor, char *device, char *instance)
{
  char address[128];
  strlcpy(address, vendor, sizeof(address));

  // Make sure the vendor has a trailing DOT.
  if(vendor[strlen(vendor)-1] != '.') {
    strlcat(address,".", sizeof(address));
  }

  if(device) {
    strlcat(address, device, sizeof(address));
  } else { // no device, compute it.
    char hostname[128];
    if(xapGetDeviceID(hostname, sizeof(hostname)) == -1) {
      strcpy(hostname, "livebox"); // still no device?  Default it.
    }
    strlcat(address, hostname, sizeof(address));
  }

  strlcat(address, ".",sizeof(address));
  strlcat(address,instance,sizeof(address));
  return strdup(address);
}

/**
 * Register an xap connection initialized from an INI file.
 */
void xapInitFromINI(
        char *section, char *vendor, char *instance, char *uid,
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

        char *sourceAddress = xapBuildAddress(vendor, NULL, instance);
        xapInit(sourceAddress, s_uid, interfaceName);
        free(sourceAddress);
        die_if(gXAP == NULL,"Failed to init xAP");
}


/// Display usage information and exit.
static void simpleUsage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF\n");
        printf("  -d, --debug            0-7\n");
        printf("  -h, --help\n");
        exit(1);
}

void simpleCommandLine(int argc, char *argv[], char **interfaceName)
{
        int c;
        static struct option long_options[] = {
          {"interface", 1, 0, 'i'},
          {"debug", 1, 0, 'd'},
          {"help", 0, 0, 'h'},
          {0, 0, 0, 0}
        };
        while(1) {
               int option_index = 0;
               c = getopt_long (argc, argv, "i:d:h",
                                long_options, &option_index);
               if (c == -1)
                   break;
               switch(c) {
               case 'i': *interfaceName = strdup(optarg); break;
               case 'd': setLoglevel(atoi(optarg)); break;
               case 'h': simpleUsage(argv[0]);  break;
               }
        }
}
