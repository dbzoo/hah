/** iServer for Joggler xAP/TCP gateway
  $Id$
 
 Copyright (c) Brett England, 2010
 
 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

#include "xap.h"
#include "tokenize.h"

const char *inifile = "/etc/xap-livebox.ini";
const char *policyResponse="<?xml version=\"1.0\" encoding=\"UTF-8\"?><cross-domain-policy xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.adobe.com/xml/schemas/PolicyFileSocket.xsd\"><allow-access-from domain=\"*\" to-ports=\"*\" secure=\"false\" /><site-control permitted-cross-domain-policies=\"master-only\" /></cross-domain-policy>";

#define ST_WAIT_FOR_MESSAGE 0
#define ST_CMD 1
#define ST_ADD_SOURCE_FILTER 2
#define ST_ADD_CLASS_FILTER 3
#define ST_XAP 4
#define ST_LOGIN 5
#define ST_DEL_SOURCE_FILTER 6
#define ST_DEL_CLASS_FILTER 7

#define BUFSIZE 2048

typedef struct _client
{
	int connected;
        int fd;
        char *ip;
        time_t connectTime;
        unsigned int txFrame;
        unsigned int rxFrame;
        char *source; // xap-heat/source from 1st heartbeat
        xAPSocketConnection *xapSocketHandler;
        char firstMessage;
        pthread_t thread;
        // Parser items
        unsigned char state; // Message state
        char ident[XAP_DATA_LEN+1]; // Identifier
        // LL
        struct _client *next;
}
Client;

int yylex();
void *yy_scan_bytes();
YYSTYPE yylval;
Client *clientList = NULL;

// Options
char *interfaceName = "eth0";
int opt_a; // authorisation mode
int opt_b; // iServer port
int opt_c = 20; // BSC Query pacing on client filter registration (ms)
int opt_e = 0;  // Inject Rx frame sequence into packet (debugging)
int opt_f = 0;  // Inject Tx frame sequence into packet (debugging)
char *password;
int lowerData = 0;

/**************************************/

/** Send data to a file descriptor (socket)
 *
 * Due to circumstances beyond our control if the kernel decides not to send
 * all the data out in one chunk make sure it gets out there.
 *
 * @param s Socket descriptor
 * @param buf Buffer to send, ZERO terminated.
 * @param len Length of buffer
 * @return -1 on data failure, -2 client disconnected during send, otherwise 0
 */
int sendAll(int s, char *buf, int len)
{
        int total = 0;        // how many bytes we've sent
        int bytesleft = len; // how many we have left to send
        int n;

        while(total < len) {
                n = send(s, buf+total, bytesleft, MSG_NOSIGNAL);
                if (n == -1) {
			// Client disconnected during send
			if(errno == EPIPE || errno == ECONNRESET || errno == ETIMEDOUT) {
				return -2;
			}
                        err_strerror("send");
                        break;
                }
                total += n;
                bytesleft -= n;
        }
        if(bytesleft) {
                error("Failed to send %d bytes", bytesleft);
        }

        return n==-1?-1:0; // return -1 on data failure, 0 on success
}

/** Send data to a Client bumping the number of frames the client has received
 * 
 * @param c Client
 * @param buf Buffer to send, ZERO terminated.
 * @param len Length of buffer
 * @return -1 on failure. otherwise 0
 */
int sendToClient(Client *c, char *buf, int len)
{
	int ret;
	
        debug("%s msg %s", c->ip, buf);
        c->rxFrame++;
	if((ret = sendAll(c->fd, buf, len)) == -2)
		c->connected = 0;
	return ret;
}

/** Rx socket handler callback for all incoming UDP packets.
*/
void incomingXapPacket(int fd, void *data)
{
	if(readXapData() > 0) {
                // OK we got the data but we don't need to process it (yet).
		if(gXAP->filterList == NULL)
			return;
		parseMsg();
		lowerData = 0;
		filterDispatch();
	}
}

/** Inject the frame number into the xAP payload.
 * Useful for debugging and making sure no packets are being lost.
 */
void injectFrameSequence(char *buf, int frame) 
{
	char seq[20];
	int len = strlen(buf);
	//Assumption: buf is of size XAP_DATA_LEN so we can shunt data around.
	if(len > XAP_DATA_LEN - sizeof(seq)) {
		warning("Not enough room for seq data");
		return;
	}
	char *lParen = strchr(buf, '}');
	if(lParen == NULL) return;
	// rxFrame+1 is the Frame is will be when sent.
	int slen = sprintf(seq,"iseq=%u\n", frame);
	memmove(lParen+slen, lParen, len-(buf-lParen));
	memcpy(lParen, seq, slen);
}

/** Outgoing packet for UDP broadcast from a client
 */
void broadcastPacket(Client *c, char *msg) {
	c->txFrame++;
	if (opt_f) injectFrameSequence(msg, c->txFrame);
	xapSend(msg);
}

/** Incoming xAP packet that got past the client filters forward to the respective client.
 * 
 * @param userData pointer to client file descriptor  
 */
void xAPtoClient(void *userData)
{
        Client *c = (Client *)userData;
        static char msg[XAP_DATA_LEN+12];
        static int msglen = 0;

	// Only build and lowercase the message once PER incoming UDP message.
        if(lowerData == 0) {
                strcpy(msg,"<xap>");
                xapLowerMessage();
                parsedMsgToRaw(&msg[5], sizeof(msg)-5);
		if(opt_e) injectFrameSequence(msg, c->rxFrame+1);
                strcat(msg,"</xap>");
                msglen = strlen(msg);
                lowerData = 1;
        }

        sendToClient(c, msg, msglen);
}

/**
 * Send a BSC query for each endpoint with subaddress Filter the client has.
 * How do we know they are BSC?
 * We don't if they aren't nobody will respond.
 *
 * @param userData pointer to client file descriptor
 */
void *sendBscQueryToFilters(void *userData)
{
        Client *c = (Client *)userData;
        xAPFilterCallback *f;
        char buff[XAP_DATA_LEN];

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        LL_FOREACH(gXAP->filterList, f) {
                if(f->user_data == c) { // Any filter for this Client.
                        // We know there is only 1 FILTER in the linked list.
                        char *key = f->filter->key;
                        char *source = f->filter->value;
                        // If this is a xap-header {source=} filter and the key
                        // has a subaddress send a BSC query to it.
                        if(strcmp(key,"source") == 0 && strchr(source,':')) {
                                snprintf(buff, XAP_DATA_LEN, "xap-header\n"
                                         "{\n"
                                         "v=12\n"
                                         "hop=1\n"
                                         "uid=%s\n"
                                         "class=xAPBSC.query\n"
                                         "source=%s\n"
                                         "target=%s\n"
                                         "}\n"
                                         "request\n"
                                         "{\n"
                                         "}\n",
                                         xapGetUID(), xapGetSource(), source);
				broadcastPacket(c, buff);
                                // So we don't saturate the bus or the client handling the reply.
                                usleep(opt_c); // microseconds
                                // Sleep is a cancelation point too but explicity check anyway.
                                pthread_testcancel();
                        }
                }
        }
        pthread_exit(NULL);
        die("Thread didn't exit?");
}

/** When the first xap-header of xap-hbeat message is seen
 * from the client we setup a directed TARGET filter and start
 * sending BSC queries for its filters.
 *
 * @param c pointer to client descriptor
 */
void firstClientMessage(Client *c)
{
        if(c->firstMessage == 0) // Already done.
                return;

        xAPFrame m;
        m.len = strlcpy((char *)m.dataPacket, c->ident, XAP_DATA_LEN);
        parseMsgF(&m);
        char *source = xapGetValueF(&m, "xap-hbeat","source");
        if(source == NULL)
                source = xapGetValueF(&m, "xap-header","source");
        if(source == NULL)
                return;

        // Stop us from being processed again
        c->firstMessage = 0;

        // Save the xAP originating source for reporting
        c->source = strdup(source);
        info("%s has source %s", c->ip, source);

        // Register a filter to send targeted messages to the client
        xAPFilter *f = NULL;
        xapAddFilter(&f,"xap-header","target", source);
        xapAddFilterAction(&xAPtoClient, f, c);

        // Send a BSC query to each registered filter with an endpoint.
        // With a large number of filters this could generate a serious amount of data.
        // As we are single threaded we won't be able to relay the result and as they
        // are UDP they will back up and be lost.  So spawn the QUERY into a thread.
        int rv = pthread_create(&c->thread, NULL, sendBscQueryToFilters, c);
        if(rv) {
                error("pthread_create: ret %d", rv);
        }
}

/** Check for an already existing callback filter for this client xap source address
 * 
 * @param source xap-header source= value.
 * @param client Client
 * @return 1 if unique, 0 otherwise 
 */
int uniqueFilter(char *source, Client *c)
{
        xAPFilterCallback *f;
        LL_FOREACH(gXAP->filterList, f) {
                if(f->callback == xAPtoClient && f->user_data == c) {
                        // We know there is only 1 FILTER in the linked list.
                        // So we can safely just check the first entry.
                        if(strcmp(f->filter->key,"source") == 0 && strcmp(f->filter->value,source) == 0) {
                                info("%s is a duplicate", source);
                                return 0;
                        }
                }
        }
        return 1;
}

/** Parse messages from the Joggler
//
// They look a litte like this.
// (src = iServer, dest = Joggler)
//  
// src: <ACL></ACL>
// dest: [[1]]<cmd>log<code>Joggler</cmd>
// src: <ACL>Joggler</ACL>
// dest: [[2]]<cmd><flt+>dbzoo.livebox.controller:relay.1</cmd>
// dest: [[3]]<cmd><flt+>dbzoo.livebox.controller:relay.2</cmd>
// ...
// dest: [[14]]<xap>msg</xap>
// src: <xap>msg</xap>
//  
// The [[\d+]] tokens are sent from the joggler flash application
// running in debug mode.  They can be disabled and aren't part of
// the protocol per se.
 *
 * @param fd Socket descriptor
 * @param msg Buffer to send, ZERO terminated.
 * @param len Length of buffer
 */
void parseiServerMsg(Client *c, unsigned char *msg, int len)
{
        int token;
        char *filterType;

        void *b = yy_scan_bytes(msg, len);
        while( c->connected && (token = yylex()) > 0) {
                switch(c->state) {
                case ST_WAIT_FOR_MESSAGE:
                        switch(token) {
                        case YY_MSG_SEQUENCE: // protocol debug token
                                info("Mesg Sequence %d", yylval.i);
                                break;
                        case YY_START_CMD:
                                c->ident[0] = 0;
                                c->state = ST_CMD;
                                break;
                        case YY_START_XAP:
                                c->ident[0] = 0;
                                c->state = ST_XAP;
                                break;
                        default:
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                case ST_LOGIN:
                        switch(token) {
                        case YY_IDENT:
                                strlcat(c->ident, yylval.s, XAP_DATA_LEN);
                                break;
                        case YY_END_CMD:
                                info("Login password %s", c->ident);
                                // If there is no authorisation or the password matched.
                                if (opt_a == 0 || strcmp(password, c->ident) == 0) {
                                        info("Authorized");
                                        // Send code back we are good to go.
                                        char *acl = "<ACL>Joggler</ACL>";
                                        sendToClient(c, acl, strlen(acl));
                                } else {
                                        info("Access denied");
                                }
                                // drop through
                        default:
                                // On failure the iServer(windows) sends <ACL></ACL> again
                                // but if the password already failed whats the point?
                                // Just wait until the client disconnects and tries again.
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                case ST_CMD:
                        switch(token) {
                        case YY_LOGIN:
                                c->state = ST_LOGIN;
                                break;
                        case YY_ADD_SOURCE_FILTER:
                                c->state = ST_ADD_SOURCE_FILTER;
                                break;
                        case YY_ADD_CLASS_FILTER:
                                c->state = ST_ADD_CLASS_FILTER;
                                break;
                        case YY_DEL_SOURCE_FILTER:
                                c->state = ST_DEL_SOURCE_FILTER;
                                break;
                        case YY_DEL_CLASS_FILTER:
                                c->state = ST_DEL_CLASS_FILTER;
                                break;
                        default:
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                case ST_ADD_SOURCE_FILTER:
                case ST_ADD_CLASS_FILTER:
                        switch(token) {
                        case YY_IDENT:
                                strlcat(c->ident, yylval.s, XAP_DATA_LEN);
                                break;
                        case YY_ADD_SOURCE_FILTER_E:
                        case YY_ADD_CLASS_FILTER_E:
                        case YY_END_CMD:
                                filterType = c->state == ST_ADD_SOURCE_FILTER ? "source" : "class";
                                info("Add %s filter %s", filterType, c->ident);
                                if(uniqueFilter(c->ident, c)) {
                                        xAPFilter *f = NULL;
                                        xapAddFilter(&f, "xap-header", filterType, c->ident);
                                        xapAddFilterAction(&xAPtoClient, f, c);
                                }
                                // drop through
                        default:
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                case ST_DEL_SOURCE_FILTER:
                case ST_DEL_CLASS_FILTER:
                        switch(token) {
                        case YY_IDENT:
                                strlcat(c->ident, yylval.s, XAP_DATA_LEN);
                                break;
                        case YY_DEL_SOURCE_FILTER_E:
                        case YY_DEL_CLASS_FILTER_E:
                        case YY_END_CMD:
                                filterType = c->state == ST_DEL_SOURCE_FILTER ? "source" : "class";
                                info("Delete %s filter %s", filterType, c->ident);
                                xAPFilterCallback *fc, *fctmp;
                                LL_FOREACH_SAFE(gXAP->filterList, fc, fctmp) {
                                        if(fc->callback == xAPtoClient &&
                                                        fc->user_data == c &&
                                                        strcmp(fc->filter->section,"xap-header") == 0 &&
                                                        strcmp(fc->filter->key,filterType) == 0 &&
                                                        strcmp(fc->filter->value, c->ident) == 0) {
                                                xapDelFilterAction(fc);
                                        }
                                }
                                // drop through
                        default:
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                case ST_XAP:
                        switch(token) {
                        case YY_IDENT:
                                strlcat(c->ident, yylval.s, XAP_DATA_LEN);
                                break;
                        case YY_END_XAP:
				broadcastPacket(c, c->ident);
                                firstClientMessage(c);
                                // drop through
                        default:
                                c->state = ST_WAIT_FOR_MESSAGE;
                        }
                        break;
                }
        }
        yy_delete_buffer(b);
}

/** Disconnect a client
 *
 * @param c Client structure
 */
void delClient(Client *c)
{
        info("Disconnecting %s / %s", c->ip, c->source);

        // The thread may not be running that's fine we don't care about the error
        if(c->firstMessage == 0)
                pthread_cancel(c->thread);

        close(c->fd);
        xapDelSocketListener(c->xapSocketHandler);

        // Delete filters callbacks for the Client
        // As the Client* is attached as user data to the callback this
        // is used to locate the filterCallback and the filters to delete.
        // TODO: Leaky xaplib2 API abstractions are showing here.
        xAPFilterCallback *fc, *fctmp;
        LL_FOREACH_SAFE(gXAP->filterList, fc, fctmp) {
                if(fc->user_data == c) {
                        xapDelFilterAction(fc);
                }
        }

        LL_DELETE(clientList, c);
        // If the client disconnected before we see a message from it c->source will still be NULL.
        if(c->source) free(c->source);
        free(c->ip);
        free(c);
}

/** Handle incoming data from the client (Joggler)
 * 
 * @param fd Client file descriptor 
 * @param data unused
 */
void clientListener(int fd, void *data)
{
        Client *c = (Client *)data;
        static unsigned char buf[XAP_DATA_LEN+1];
        int bytes;

        // Drain the socket of data.
        while( (bytes = recv(fd, buf, XAP_DATA_LEN, MSG_DONTWAIT)) > 0) {
                parseiServerMsg(c, buf, bytes);
        }

        if(bytes == 0 || c->connected == 0) { // Disconnected client
                delClient(c);
        }
}
/** Add and handle a client connection
 * 
 * @param fd Client socket file descriptor
 * @param remote sockaddr of the client
 * @return Client object
 */
Client *addClient(int fd, struct sockaddr_in *remote)
{
        char *myip = inet_ntoa(remote->sin_addr);
        Client *c = (Client *)calloc(1, sizeof(Client));
        if(c == NULL)
        {
                alert("Out of memory - Adding client from %s", myip);
                return NULL;
        }
	c->connected = 1;
        c->firstMessage = 1;
        c->fd = fd;
        c->ip = strdup(myip);
        c->state = ST_WAIT_FOR_MESSAGE;
        c->connectTime = time(NULL);
        c->xapSocketHandler = xapAddSocketListener(fd, &clientListener, c);
        info("Connection from %s", c->ip);
        LL_PREPEND(clientList, c);

        // Initiate communication with client.
        char *init = "<ACL></ACL>";
	if(sendToClient(c, init, strlen(init)) != 0) {
		delClient(c);
		return NULL;
	}	
        return c;
}

/** iServer connection handler on port 9996
 *
 * @param fd iServer descriptor
 * @param data unused 
 */
void serverListener(int fd, void *data)
{
        socklen_t addr_size;
        struct sockaddr_in remote;

        addr_size = sizeof(remote);
        int client = accept (fd, (struct sockaddr *)&remote, &addr_size);

        if(client < 0) {
                err_strerror("accept");
                return;
        }

        addClient(client, &remote);
}

/** Micro WEB server
 *  Allows you to see what clients have connected and some internal statistics
 *
 * @param fd Socket file descriptor
 * @param data unused
 */
void webServerHandler(int fd, void *data)
{
        socklen_t addr_size;
        struct sockaddr_in remote;
        int ret, i, len;
        static char buffer[BUFSIZE+1];
        Client *c;

        addr_size = sizeof(remote);
        int client = accept (fd, (struct sockaddr *)&remote, &addr_size);

        ret = read(client, buffer, BUFSIZE);
        if(ret == 0 || ret == -1) { // read failure
                err_strerror("read failure");
                return;
        }
        if(ret > 0 && ret < BUFSIZE)
                buffer[ret] = 0;
        else
                buffer[0] = 0;

        if(strncmp(buffer,"GET ", 4) && strncmp(buffer, "get ",4))
                error("Only simply GET operation is supported");

        // Build response
        strcpy(buffer,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
        if(clientList == NULL) {
                len = strlcat(buffer,"<i>No connected Clients</i>",BUFSIZE);
                sendAll(client, buffer, len);
        } else {
                len = strlcat(buffer,"<table><tr><th>FD</th><th>IP</th><th>Source</th><th>Tx</th><th>Rx</th><th>/min</th></tr>",BUFSIZE);
                sendAll(client, buffer, len);
                LL_FOREACH(clientList, c) {
                        int elapsedMinutes = (time(NULL) - c->connectTime)/60;
                        int totalFrames = c->rxFrame + c->txFrame;
                        float flowRate = elapsedMinutes == 0 ? totalFrames : (float)totalFrames/(float)elapsedMinutes;

                        len = snprintf(buffer, BUFSIZE,"<tr><td>%d</td><td>%s</td><td>%s</td><td align=\"right\">%u</td><td align=\"right\">%u</td><td align=\"right\">%5.1f</td></tr>",
                                       c->fd, c->ip, c->source, c->txFrame, c->rxFrame, flowRate);
                        sendAll(client, buffer, len);
                }
                sendAll(client, "</table>", 8);
        }

        close(client);
}

/** Policy service handler listening on port 843
 * 
 * @param fd Socket file descriptor 
 * @param data unused
 */
void flashPolicyServerHandler(int fd, void *data)
{
        socklen_t addr_size;
        struct sockaddr_in remote;
        char buf[BUFSIZE+1];

        addr_size = sizeof(remote);
        int client = accept (fd, (struct sockaddr *)&remote, &addr_size);

        if(client < 0) {
                err_strerror("accept");
        } else {
                info("Flash policy file request from %s", inet_ntoa(remote.sin_addr));
                int bytes = recv(client, buf, BUFSIZE, 0);
                buf[bytes] = 0;
                if (strcmp("<policy-file-request/>", buf) == 0) {
                        sendAll(client, (char *)policyResponse, strlen(policyResponse)+1);
                } else {
                        notice("Unrecognized flash policy request %s", buf);
                }
                close(client);
        }
}

/** Bind to a port and open a socket
 * 
 * @param port port to bind to and open
 * @return socket file descriptor
 */
int serverBind(int port)
{
        struct sockaddr_in myaddr;     // server address
        int listener;     // listening socket descriptor
        int yes=1;        // for setsockopt() SO_REUSEADDR, below

        if(port <= 0) {
                die("Invalid port %d", port);
        }

        // get the listener
        if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
                die_strerror("socket");
        }

        // lose the pesky "address already in use" error message
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                        sizeof(int)) == -1) {
                die_strerror("setsockopt");
        }

        // bind
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = INADDR_ANY;
        myaddr.sin_port = htons(port);
        memset(&(myaddr.sin_zero), '\0', 8);
        if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
                // don't make it fatal so we can run as a non-root user if port < 1024
                crit_strerror("Bind to port %d", port);
                return -1;
        }

        // listen
        if (listen(listener, 5) == -1) {
                die_strerror("listen");
        }
        return listener;
}

void setupXAPini()
{
        xapInitFromINI("iserver","dbzoo.livebox","iServer","00DE",interfaceName,inifile);

        opt_a = ini_getl("iserver","authmode",0,inifile);
        opt_b = ini_getl("iserver","port",9996,inifile);
        opt_c = ini_getl("iserver","pacing",20,inifile);
        opt_e = ini_getl("iserver","rxseq",0,inifile);
        opt_f = ini_getl("iserver","txseq",0,inifile);

        if(opt_a) {
                password = getINIPassword("iserver","passwd", (char *)inifile);
                if(strlen(password) == 0) {
                        die("password hasn't been setup and we are in authorisation mode");
                }
        }
}

/// Display usage information and exit.
static void usage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF     Default %s\n", interfaceName);
        printf("  -d, --debug            0-7\n");
        printf("  -a                     Enable authorisation mode (default off)\n");
        printf("  -b PORT                Listening port (default 9996)\n");
        printf("  -c pacing              BSC Query pacing (default 50ms)\n");
        printf("  -e                     Inject Rx seq# into client xap-header messages\n");
        printf("  -f                     Inject Tx seq# into UDP xap-header messages\n");
        printf("  -p PASSWD              Authorisation password\n");
        printf("  -h, --help\n");
        exit(1);
}

/** MAIN
*/
int main(int argc, char *argv[])
{
        int i;
        printf("\niServer for xAP v12\n");
        printf("Copyright (C) DBzoo 2010\n");

        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        interfaceName = argv[++i];
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
                        usage(argv[0]);
                }
        }

        setupXAPini();

        // Command line override for INI
        for(i=0; i<argc; i++) {
                if(strcmp("-a", argv[i]) == 0) {
                        opt_a = 1;
                } else if(strcmp("-p", argv[i]) == 0) {
                        password = strdup(argv[++i]);
                } else if(strcmp("-b", argv[i]) == 0)  {
                        opt_b = atoi(argv[++i]);
                } else if(strcmp("-c", argv[i]) == 0)  {
                        opt_c = atoi(argv[++i]);
                } else if(strcmp("-e", argv[i]) == 0)  {
                        opt_e = 1;
                } else if(strcmp("-f", argv[i]) == 0)  {
                        opt_f = 1;
                }
        }
        if(opt_c < 10)
                opt_c = 10; // 10ms minimum
        opt_c *= 1000; // convert to microseconds for sleep() call.

	// Remove the default UDP handler and use our own.
	xapDelSocketListener(xapFindSocketListenerByFD(gXAP->rxSockfd));
	xapAddSocketListener(gXAP->rxSockfd, incomingXapPacket, NULL);
	
        xapAddSocketListener(serverBind(opt_b), &serverListener, NULL);
        // The Joggler never makes such a request - MS windows flash will.
        //http://www.adobe.com/devnet/flashplayer/articles/socket_policy_files.html
        xapAddSocketListener(serverBind(843), &flashPolicyServerHandler, NULL);
        xapAddSocketListener(serverBind(78), &webServerHandler, NULL);
        xapProcess();
        return 0; // never reached
}
