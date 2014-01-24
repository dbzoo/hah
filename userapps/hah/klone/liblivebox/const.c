/* $Id$
*/
#include <string.h>
#include <ctype.h>
#include "const.h"
const_t con;

char *u_strndup(char *, int);

// Called once when the webserver starts up
const_init() {
	 int size;
	 char *file;
	 int ret = u_load_file("/etc_ro_fs/build", 15, &file, &size); // 0 on success, ~0 error
	 
	 if(ret) {
	   ret = u_load_file("/etc/xap.d/build", 15, &file, &size);
	 }	 
	 if(ret) {
		  con.build = strdup("?");
	 } else {
		  con.build = u_strndup(file, size);
		  int len = strlen(con.build)-1;
		  int i;
                  for(i=len; isspace(con.build[i]); i--)
                     con.build[i] = '\0';
		  u_free(file);
	 }

	// What is our IP?
/*
	 struct ifreq i_interface;
	 int fd = socket(AF_INET, SOCK_DGRAM, 0);
	 memset((char*)&i_interface, sizeof(i_interface),0);
	 i_interface.ifr_addr.sa_family = AF_INET; 
	 if(ioctl(fd, SIOGIFADDR, &i_interface)) {
		  strcpy(IP,"?.?.?.?");
	 } else {
		  strcpy(IP, inet_ntoa(((struct sockaddr_in *)&i_interface.ifr_addr)->sin_addr));
	 }
*/
}
