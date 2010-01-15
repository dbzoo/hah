/* $Id: modem.c 3 2009-11-09 12:19:52Z brett $
 */

#ifdef IDENT
#ident "@(#) $Id: modem.c 3 2009-11-09 12:19:52Z brett $"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "xapdef.h"
#include "debug.h"

int g_serial_fd;

#define READ_SLEEP 10000
#define MAX_BUF 2048
#define optz(_n,_l)     (buf+buf_len-(((_n)+(_l)>buf_len)?buf_len:(_n)+(_l)))

#define MODEM_INIT "AT&FE0+CMGF=1;+CNMI=2,1,0,0,0\r"

int put_command(char *cmd, int cmd_len, char *answer, int max, int timeout, char *exp_end) {
	 in("put_command");

	 int timeoutcounter = 0;
/*
	 // Check if the FD is "clean" for reading
	 int status;
	 ioctl(g_serial_fd, TIOCMGET, &status);
	 while(!(status & TIOCM_CTS)) {
		  usleep(READ_SLEEP);
		  timeoutcounter++;
		  ioctl(g_serial_fd, TIOCMGET, &status);
		  if(timeoutcounter >= timeout) {
			   if(g_debuglevel>2) printf("Modem is not clear to send\n");
			   goto error;
		  }
	 }
*/
	 // Send the command to the modem
	 if(g_debuglevel>4) printf("Serial Tx: %.*s\n", cmd_len, cmd);
	 write(g_serial_fd, cmd, cmd_len);
	 tcdrain(g_serial_fd);

	 // Read from the modem
	 int buf_len = 0;
	 int available;
	 int n;
	 static char buf[MAX_BUF];
	 char *foo, *pos, *answer_e, *answer_s;
	 int exp_end_len;

	 exp_end_len = exp_end ? strlen(exp_end) : 0;
	 answer_s = buf;
	 answer_e = 0;

	 do {
		  // try to read some bytes
		  ioctl(g_serial_fd, FIONREAD, &available);
		  if(available < 1) {
			   usleep(READ_SLEEP);
			   timeoutcounter++;
			   ioctl(g_serial_fd, TIOCMGET, &available);
		  }
		  if(available > 0) {
			   n = (available > MAX_BUF-buf_len-1)?MAX_BUF-buf_len-1:available;
			   // read data
			   n = read(g_serial_fd, buf+buf_len, n);
			   if(n<0) {
					if(g_debuglevel>2) printf("error reading from modem: %s\n", strerror(errno));
					goto error;
			   }
			   if(n) {
					buf_len += n;
					buf[buf_len] = 0;
					foo = pos = 0;
					if ( (!exp_end && ((pos = strstr(optz(n,4),"OK\r\n"))
									   || (foo=strstr(optz(n,5),"ERROR"))))
						 || (exp_end && (pos=strstr(optz(n,exp_end_len),exp_end)) )) {

						 if(!foo || (foo=strstr(foo+5,"\r\n"))) {
							  answer_e = foo?foo+2:(pos+(exp_end?exp_end_len:4));
							  timeoutcounter = timeout;
						 }
					}
			   }
		  }
	 } while(timeoutcounter < timeout);

	 if (!answer_e)
		  answer_e = buf+buf_len;

	 if(answer && max) {
		  n = max-1<answer_e-answer_s?max-1:answer_e-answer_s;
		  memcpy(answer,answer_s,n);
		  answer[n] = 0;
	 }

	 // show modem command
	 if(g_debuglevel>2)
		  printf("Serial Rx: %s \n", answer);
	 buf_len = 0;

	 out("put_command");
	 return answer_e-answer_s;

error:
	 out("put_command");
	 return 0;
}

void setup_serial_port() {
	 in("setup_serial_port");
	 struct termios newtio;

	 printf("\nUsing serial device %s\n\n", g_serialport);
	 g_serial_fd = open(g_serialport, O_RDWR | O_NOCTTY );
	 if (g_serial_fd < 0)
	 {
		  printf("Unable to open the serial port %s\n",g_serialport);
		  exit(4);
	 }

	 bzero(&newtio, sizeof(newtio));
	 newtio.c_cflag = B19200 | CRTSCTS | CS8 | CLOCAL | CREAD | O_NDELAY;
	 // uncomment next line to disable hardware handshake
	 newtio.c_cflag &= ~CRTSCTS;
	 newtio.c_iflag = IGNPAR;
	 newtio.c_oflag = 0;
	 newtio.c_lflag = 0;
	 newtio.c_cc[VTIME] = 1;
	 newtio.c_cc[VMIN] = 0; /* no blocking read */
	 tcflush(g_serial_fd, TCIOFLUSH);
	 tcsetattr(g_serial_fd, TCSANOW, &newtio);

	 out("setup_serial_port");
}

int initmodem() {
	 int retries = 0;
	 int success = 0;
	 char answer[100];

 	 in("initmodem");
	 do {
		  retries++;
		  put_command("AT+CREG?\r", 9, answer, sizeof(answer), 100, 0);
		  if(strchr(answer,'1')) {
			   if(g_debuglevel>2) printf("Modem is registered to the network\n");
			   success = 1;
		  } else if (strchr(answer,'2')) {
			   if(g_debuglevel>2) printf("Modem seems to try to reach the network!\n");
			   retries--;
			   sleep(2);
		  } else if(strchr(answer,'5')) {
			   if(g_debuglevel>2) printf("Modem is registered to a roaming partner network\n");
			   success = 1;
		  } else if(strstr(answer,"ERROR")) {
			   if(g_debuglevel>2) printf("Ignoring modem does not support +CREG command\n");
			   success = 1;
		  } else {
			   if(g_debuglevel>2) printf("initmodem: Waiting 2 secs before retrying\n");
			   sleep(2);
		  }
	 } while((success==0) && (retries<20));

	 if(success == 0)
		  goto error;

	 put_command(MODEM_INIT, strlen(MODEM_INIT), answer,sizeof(answer), 100, 0);
	 if(strstr(answer,"ERROR")) {
		  if(g_debuglevel>2) {
			   printf("Failed to initalize\n");
			   printf("cmd [%s] returned ERROR\n", MODEM_INIT);
		  }
		  goto error;
	 }

 	 out("initmodem");
	 return 0;
error:
 	 out("initmodem");
	 return -1;
}

int checkmodem() {
	 char answer[500];
	 int status = 0;
	 in("checkmodem");
	 put_command("AT+CREG?\r", 9, answer, sizeof(answer), 100, 0);
	 if(!strchr(answer,'1')) {
		  if(g_debuglevel>2) printf("Modem is not registered to the network\n");
		  status=initmodem();
	 }
	 out("checkmodem");
	 return status;
}
