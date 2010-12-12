// iServer client

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "xap.h"

char *interfaceName = "eth0";
const int port=9996;
int id = 1;

void toServer(int fd, char *msg) {
	int i = strlen(msg);
	//write(0, msg, i);
	write(fd, msg, strlen(msg));
}

void fromServer(int fd) {
	char buffer[512];
	int i;
	while( (i = recv(fd, buffer, 512, MSG_DONTWAIT)) > 0) {
		//write(0, buffer, i);
	}
}

void connectToServer(char *ip) {
	struct sockaddr_in serv;
	int fd;

	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die_strerror("socket()");

	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(ip);
	serv.sin_port = htons(port); // iServer listens here
	if(connect(fd, (struct sockaddr *)&serv, sizeof(serv)) < 0)
		die_strerror("check iServer is running");
	
	fromServer(fd);
	toServer(fd,"<cmd>log<code></cmd>");
	fromServer(fd);
	toServer(fd,"<cmd><flt+>dbzoo.data.load</cmd>");

	while(1) {
		fromServer(fd);
	}
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Usage: %s iServerIP\n", argv[0]);
		exit(1);
	}
	char *iServer = argv[1];

	xapInit("dbzoo.data.load","FF777700",interfaceName);
	connectToServer(iServer);	
}
