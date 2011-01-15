/* $Id$

   We act as a server so that external calls can be made from non XAP enabled
   applications to find out the current state of the endpoints.

   This is used to satisfy local web server page queries without having to 
   XAP enable the webserver too.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include "bsc.h"
#include "log.h"
#include "serial.h"

extern bscEndpoint *endpointList;


/* CMD: query <endpoint>
 * Ret: <state>
 *
 * Example
 *   Cmd: query rf.1
 *   Ret: on off
 */
static char *msg_query(char *args) {
	debug("endpoint: %s", args);

	char *name = strtok(args,".");
	char *subaddr = strtok(NULL,"");

	bscEndpoint *endpoint = bscFindEndpoint(endpointList, name, subaddr);
	if (endpoint) {
		if(endpoint->type == BSC_BINARY)
			return bscStateToString(endpoint);
		else
			return endpoint->text;
	}
	return "bad endpoint";
}

/* Relay action
 *
 * CMD: action <endpoint>,<state> [<endpoint>,<state>...]
 * RET: "ok" or "error message"
 *
 * State can be 1:on,true,yes,1 - 0:off,false,no,0
 *
 * Example
 *   Cmd: action rf.1,on rf.2,off
 *   Ret: ok
 */
static char *msg_action(char *arg) {
	char *tuple_ptr;
	char *tuple = strtok_r(arg," ", &tuple_ptr);

	while(tuple) {
		char *name = strtok(tuple,".");
		char *subaddr = strtok(NULL,",");
		char *state = strtok(NULL," ");
		
		info("name=%s subaddr=%s state=%s", name, subaddr, state);
		if (name == NULL || state == NULL) {
			return "malformed argument";
		}
		
		bscEndpoint *endpoint = bscFindEndpoint(endpointList, name, subaddr);
		if (endpoint) {
			bscSetState(endpoint, bscDecodeState(state)); // Take action
			bscSendCmdEvent(endpoint);
		} else {
			err("bad endpoint: %s", arg);
		}
		// Next action pair
		tuple = strtok_r(NULL," ",&tuple_ptr);
	}
	return "ok";
}

/* Break into: <cmd> <args>
   Arguments will be procesed by the appropriate command handler.
*/
static char *msg_handler(char *a_cmd) {
	char *cmd = strtok(a_cmd," ");
	char *args = strtok(NULL, "");

	// Process command: each will return either "ok" or an error message.
	if(strcmp(cmd,"query") == 0) {
		return msg_query(args);
	} else if(strcmp(cmd,"version") == 0) {
		static char rev[4];
		sprintf(rev,"%d.%d", firmwareMajor(), firmwareMinor());
		return rev;
	} 
	else if(strcmp(cmd,"action") == 0) {
		return msg_action(args);
	} 
	else if(strcmp(cmd,"lcd") == 0) {
		bscEndpoint *lcd = bscFindEndpoint(endpointList, "lcd", NULL);
		if(lcd) {
			bscSetText(lcd, args);
			bscSendCmdEvent(lcd);
		}
		return "ok";
	} 
	else // Invalid command
		return "fail";
}

/**********************************************************************
 * Server SOCKET setup.
 */

static int sendall(int s, char *buf, int *len)
{
	 int total = 0;        // how many bytes we've sent
	 int bytesleft = *len; // how many we have left to send
	 int n;

	 while(total < *len) {
		  n = send(s, buf+total, bytesleft, 0);
		  if (n == -1) { break; }
		  total += n;
		  bytesleft -= n;
	 }

	 *len = total; // return number actually sent here
	 return n==-1?-1:0; // return -1 on failure, 0 on success
}


/* Server message handler */
static void svr_process( int fd ) {
	int n;
	char buff[512];

	while((n = recv(fd, buff, sizeof(buff), 0)) != 0) {		
		 debug("Server got <%s>", buff);
		 char *reply = msg_handler(buff);
		 debug("Server sent <%s>", reply);
		 int len = strlen(reply);
		 len ++; // include NULL terminator too.
		 // What happens if we can't send all the packets
		 // due to a send() error !?
		 sendall(fd, reply, &len);
	}
}

/* Called when data is detected.
   Return FD is passed to svr_process() 
*/
static int svr_accept(int a_fd)
{
	int newfd;
	socklen_t addr_size;
	struct sockaddr_in remote;

	addr_size = sizeof(remote);
	newfd = accept (a_fd, (struct sockaddr *)&remote, &addr_size);
	debug("got connection from %s", inet_ntoa(remote.sin_addr));

	return newfd;
}

/* Call once to setup the socket */
int svr_bind(int port) {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results
	char s_port[6];

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	snprintf(s_port, sizeof(s_port), "%d", port);
	if ((status = getaddrinfo(NULL, s_port, &hints, &servinfo)) != 0) {
		die_strerror("getaddrinfo");
	}

	int sockfd;
	int yes=1;
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		 if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			  warning("socket");
			  continue;
		 }
		 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			 die_strerror("setsockopt");
		 }
		 if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) { /* success */
			  break;
		 }
		 close(sockfd);
	}
	die_if(p == NULL, "Could not bind to port %d\n", port);
	
	freeaddrinfo(servinfo); /* No longer needed */
	
	if (listen(sockfd, 5) == -1) {
		die_strerror("listen");
	}
	return sockfd;
}


void webHandler(int fd, void *data) {
	int in_sockfd = svr_accept(fd);
	if (in_sockfd == -1) {
		return;
	}
	svr_process(in_sockfd);
	close(in_sockfd);
}
