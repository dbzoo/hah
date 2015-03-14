/*  Livebox Communication protocol
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// For standalone development code with IP of the LIVEBOX.
//#define IP "192.168.1.131"
// When PRODUCTION we use the LOOPBACK IP
#define IP "127.0.0.1"

#define PORT "79"
#define BUF_SIZE 1024
char buf[BUF_SIZE];

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

char *send_cmd(char *cmd) {
	 struct addrinfo hints;
	 struct addrinfo *result, *rp;
	 int sfd, s, j;
	 size_t len;
	 ssize_t nread;

	 //printf("cmd: %s\n", cmd);
	 /* Obtain address(es) matching host/port */
	 memset(&hints, 0, sizeof(struct addrinfo));
	 hints.ai_family = AF_INET;
	 hints.ai_socktype = SOCK_STREAM;
	 hints.ai_flags = 0;

	 s = getaddrinfo(IP, PORT, &hints, &result);
	 if (s != 0) {
		  //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		  return "getaddrinfo: fail";
	 }
	 for (rp = result; rp != NULL; rp = rp->ai_next) {
		  sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		  if (sfd == -1)
			   continue;
		  if (connect(sfd, rp->ai_addr, rp->ai_addrlen) < 0) {
			   close(sfd);
			   continue;
		  }			   
		  break;                  /* Success */
	 }

	 if (rp == NULL) {               /* No address succeeded */
		  return "?Err?";
	 }

	 freeaddrinfo(result);           /* No longer needed */

	 /* Send command as message and wait for a response */
	 len = strlen(cmd);
	 len++; // include ZERO terminator
	 if (sendall(sfd, cmd, &len) == -1) {
		  //fprintf(stderr, "partial/failed send.  Only sent %d/%d\n", len, strlen(argv[i]));
		  return "partial send";
	 }
	 nread = recv(sfd, buf, BUF_SIZE, 0);
	 buf[nread] = 0; // Always terminate
	 close(sfd);

	 return buf;
}

char *query(char *type, int instance) {
	 char buf[100];
	 sprintf(buf,"query %s.%d", type, instance);
	 return send_cmd(buf);
}

void getAVRfirmwareVersion(int *major, int *minor) {
	char *AVRfirmware = send_cmd("version");
	if(strcmp("?Err?", AVRfirmware) == 0) {
		*major = 1;
		*minor = 0;
		return;
	}

	char *dot = strchr(AVRfirmware,'.');
	if(dot == NULL) {
		*major = atoi(AVRfirmware);
		*minor = 0;
	} else {
		*dot = '\0';
		*major = atoi(AVRfirmware);
		*minor = atoi(dot+1);
	}
}
