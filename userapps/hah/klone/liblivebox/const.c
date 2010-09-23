/* $Id$
*/
#include "const.h"
const_t con;

// Called once when the webserver starts up
const_init() {
	 con.inifile = strdup("/etc/xap-livebox.ini");

	 int size;
	 char *file;
	 if(u_load_file("/etc_ro_fs/build", 15, &file, &size)) {
		  con.build = strdup("?");
	 } else {
		  con.build = u_strndup(file, size);
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
