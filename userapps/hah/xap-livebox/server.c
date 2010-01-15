/* $Id: server.c 33 2009-11-10 09:56:39Z brett $

   We act as a server so that external calls can be made from non XAP enabled
   applications to find out the current state of the endpoints.

   This is used to satisfy local web server page queries without having to 
   XAP enable the webserver too.
*/
#include "xapdef.h"
#include "appdef.h"
#include "server.h"
#include "debug.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define DEBUG

extern endpoint_t endpoint_s[];

/* CMD: query <endpoint> [<endpoint>...]
 * Ret: <state> [<state>...]
 *
 * Example
 *   Cmd: query rf.1 rf.2
 *   Ret: on off
 */
static char g_ret[256]; // Not re-entrant.
static char *msg_query(char *args) {
	endpoint_t *endpoint;
	char *arg = strtok(args," ");

	memset(g_ret, 0, sizeof(g_ret));
	while(arg != NULL) {
		// The LCD can't be processed like the other endpoints
		// as it may itself contain spaces which will make
		// processing on the other end difficult.
		// If its found its STATE will be returned immediately.
		if(strcmp("lcd", arg) == 0)
			return g_lcd_text;

		if(g_debuglevel >=4) printf("endpoint: %s\n", arg);
		endpoint = find_endpoint(arg);
		if (endpoint == NULL) return "bad endpoint";

		// Prepend a space to all but the first result
		if(g_ret[0]) {
			strlcat(g_ret, " ", sizeof(g_ret));
		}
		strlcat(g_ret, endpoint->state, sizeof(g_ret));
		arg = strtok(NULL," ");
	}
	return g_ret;
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
	// Break the arg into tuples the process each
	char *tuple_ptr;
	char *tuple = strtok_r(arg," ", &tuple_ptr);
		
	while( tuple != NULL ) {
		char *name = strtok(tuple, ",");
		char *state = strtok(NULL,",");

		// Validate arguments
		// State validation occurs in cmd_relay()
		if (name == NULL || state == NULL) {
			return "malformed argument";
		}
		endpoint_t *endpoint = find_endpoint(name);
		if (endpoint == NULL) return "bad endpoint";

		// Take action
		if(cmd_relay(endpoint, state) == -1) {
			return "invalid command pair";
		}
		// Next action pair
		tuple = strtok_r(NULL," ",&tuple_ptr);
	}
	return "ok";
}

static char *msg_lcd(char *msg) {
	 cmd_lcd(msg);
	 return "ok";
}

/* Break into: <cmd> <args>
   Arguments will be procesed by the appropriate command handler.
*/
static char *msg_handler(char *a_cmd) {
	char *cmd = strtok(a_cmd," ");
	char *args = strtok(NULL,"");

	// Everything needs at least 1 argument
	if(!(args && *args)) {
		return "missing argument";
	}

	// Process command: each will return either "ok" or an error message.
	if(strcmp(cmd,"query") == 0) 
	{
		return msg_query(args);
	} 
	else if(strcmp(cmd,"action") == 0) 
	{
		return msg_action(args);
	} 
	else if(strcmp(cmd,"lcd") == 0) 
	{
		 return msg_lcd(args);
	} 
	else // Invalid command
		return "fail";
}

/**********************************************************************
 * Server SOCKET setup.
 */

int sendall(int s, char *buf, int *len)
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
void svr_process( int fd ) {
	int n;
	char buff[512];

	while((n = recv(fd, buff, sizeof(buff), 0)) != 0) {		
		 if(g_debuglevel >= 4) printf("Server got <%s>\n", buff);
		 char *reply = msg_handler(buff);
		 if(g_debuglevel >= 4) printf("Server sent <%s>\n", reply);
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
int svr_accept(int a_fd)
{
	int newfd;
	socklen_t addr_size;
	struct sockaddr_in remote;

	addr_size = sizeof(remote);
	newfd = accept (a_fd, (struct sockaddr *)&remote, &addr_size);

	if(g_debuglevel >= 4)
		 printf("server: got connection from %s\n", inet_ntoa(remote.sin_addr));

	return newfd;
}

/* Call once to setup the socket */
int svr_bind(int port) {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;  // will point to the results
	in("svr_bind");

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		 fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		 exit(1);
	}

	int sockfd;
	int yes=1;
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
		 if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			  perror("server: socket");
			  continue;
		 }
		 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			  perror("setsockopt");
			  exit(1);
		 }
		 if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) { /* success */
			  break;
		 }
		 close(sockfd);
    }
	if (p == NULL) { /* No address succeeded */
		 fprintf(stderr,"Could not bind to port %s\n", PORT);
		 exit(1);
	}
	freeaddrinfo(servinfo); /* No longer needed */

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(2);
	}
	out("svr_bind");
	return (sockfd);
}
