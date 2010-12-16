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

#define MIN(a,b) (a)<(b)?(a):(b)

/* dump a line (16 bytes) of memory, starting at pointer ptr for len
   bytes */

void ldump(char *ptr, unsigned int len) {
  if ( len ) {
    int  i;
    char c;
    len=MIN(16,len);
    printf("%10p ",ptr);

    for( i = 0 ; i < len ; i++ )  /* Print the hex dump for len
                                     chars. */
      printf("%3x",(*(ptr+i)&0xff));

    for( i = len ; i < 16 ; i++ ) /* Pad out the dump field to the
                                     ASCII field. */
      printf("   ");

    printf(" ");
    for( i = 0 ; i < len ; i++ ) { /* Now print the ASCII field. */
      c=0x7f&(*(ptr+i));      /* Mask out bit 7 */

      if (!(isprint(c))) {    /* If not printable */
        printf(".");          /* print a dot */
      } else {
        printf("%c",c);  }    /* else display it */
    }
  }
  printf("\n");
}

/* Print out a header for a hex dump starting at address st. Each
   entry shows the least significant nybble of the address for that
   column. */

void head(long st) {
  int i;
  printf("    addr       ");
  for ( i = st&0xf ; i < (st&0xf)+0x10 ; i++ )
    printf("%3x",(i&0x0f));
  printf(" | 7 bit ascii. |\n");
}


/* Dump a region of memory, starting at ptr for ct bytes */

void dump(char *ptr, unsigned int ct) {
  if ( ct ) {
    int i;

    /*  preliminary info for user's benefit. */
    printf("Start is %10p, Count is %5x, End is %10p\n",
           ptr, ct, ptr + ct);
    head((long)ptr);
    for ( i = 0 ; i <= ct ; i = i+16 , ptr = ptr+16 )
      ldump(ptr,(MIN(16,(ct-i))));
  }
  printf("\n");
}

void toServer(int fd, char *msg) {
	int i = strlen(msg);
	//write(0, msg, i);
	write(fd, msg, strlen(msg));
}

void fromServer(int fd) {
	char buffer[512];
	int i;
	while( (i = recv(fd, buffer, 512, MSG_DONTWAIT)) > 0) {
		dump(buffer, i);
	}
}

void connectToServer(char *ip, char *filter) {
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
	toServer(fd,"<cmd><flt+>");
	toServer(fd,filter);
	toServer(fd,"</cmd>");

	while(1) {
		fromServer(fd);
	}
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Usage: %s iServerIP Filter\n", argv[0]);
		exit(1);
	}
	char *iServer = argv[1];
	char *filter = argv[2];

	xapInit("dbzoo.data.load","FF777700",interfaceName);
	connectToServer(iServer, filter);	
}
