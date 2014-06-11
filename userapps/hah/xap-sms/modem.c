/* $Id$
 */

#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <strings.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "xap.h"
#include "modem.h"

int g_serial_fd;

#define READ_SLEEP 10000
#define MAX_BUF 2048
#define optz(_n,_l)     (buf+buf_len-(((_n)+(_l)>buf_len)?buf_len:(_n)+(_l)))

#define MODEM_INIT "AT&FE0+CMGF=1;+CNMI=2,1,0,0,0\r"

int put_command(char *cmd, int cmd_len, char *answer, int max, int timeout, char *exp_end)
{
        int timeoutcounter = 0;
        // Send the command to the modem
        info("Serial Tx: %.*s", cmd_len, cmd);
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
                        ioctl(g_serial_fd, FIONREAD, &available);
                }
                if(available > 0) {
                        n = (available > MAX_BUF-buf_len-1)?MAX_BUF-buf_len-1:available;
                        // read data
                        n = read(g_serial_fd, buf+buf_len, n);
                        if(n<0) {
                                err_strerror("error reading from modem");
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
        info("Serial Rx: %s", answer);
	
        return answer_e-answer_s;

error:
        return 0;
}

static speed_t getBaud() {
    long n = ini_getl("sms", "baud", 19200, inifile);
    speed_t speed;

    switch (n)
      {
      case 50: speed = B50; break;
      case 75: speed = B75; break;
      case 110: speed = B110; break;
      case 134: speed = B134; break;
      case 150: speed = B150; break;
      case 200: speed = B200; break;
      case 300: speed = B300; break;
      case 600: speed = B600; break;
      case 1200: speed = B1200; break;
      case 1800: speed = B1800; break;
      case 2400: speed = B2400; break;
      case 4800: speed = B4800; break;
      case 9600: speed = B9600; break;
      case 19200: speed = B19200; break;
      case 38400: speed = B38400; break;
      case 57600: speed = B57600; break;
      case 115200: speed = B115200; break;
      default:
	die("Invalid speed %d", n);
      }

    return speed;
}

int setup_serial_port()
{
        struct termios newtio;
        char serialPort[32];

        ini_gets("sms","usbserial","/dev/ttyUSB0", serialPort, sizeof(serialPort), inifile);
        info("Using serial device %s", serialPort);

        g_serial_fd = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY);
        die_if(g_serial_fd < 0, "Unable to open the serial port %s",serialPort);

	if(flock(g_serial_fd, LOCK_EX | LOCK_NB) == -1) {
	  close(g_serial_fd);
	  die_strerror("Serial port %s in use", serialPort);
	}

        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag = getBaud() | CRTSCTS | CS8 | CLOCAL | CREAD | O_NDELAY;
        // uncomment next line to disable hardware handshake
        newtio.c_cflag &= ~CRTSCTS;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VTIME] = 1;
        newtio.c_cc[VMIN] = 0; /* no blocking read */
        tcflush(g_serial_fd, TCIOFLUSH);
        tcsetattr(g_serial_fd, TCSANOW, &newtio);

        return g_serial_fd;
}

int initmodem()
{
        int retries = 0;
        int success = 0;
        char answer[100];

        do {
                retries++;
                put_command("AT+CREG?\r", 9, answer, sizeof(answer), 100, 0);
                if(strchr(answer,'1')) {
                        info("Modem is registered to the network");
                        success = 1;
                } else if (strchr(answer,'2')) {
                        info("Modem seems to try to reach the network!");
                        retries--;
                        sleep(2);
                } else if(strchr(answer,'5')) {
                        info("Modem is registered to a roaming partner network");
                        success = 1;
                } else if(strstr(answer,"ERROR")) {
                        info("Ignoring modem does not support +CREG command");
                        success = 1;
                } else {
                        info("initmodem: Waiting 2 secs before retrying");
                        sleep(2);
                }
        } while((success==0) && (retries<20));

        if(success == 0)
                goto error;

        put_command(MODEM_INIT, strlen(MODEM_INIT), answer,sizeof(answer), 100, 0);
        if(strstr(answer,"ERROR")) {
                err("Failed to initalize");
                err("cmd [%s] returned ERROR", MODEM_INIT);
                goto error;
        }
        return 0;
error:
        return -1;
}

int checkmodem()
{
        char answer[500];
        int status = 0;
        put_command("AT+CREG?\r", 9, answer, sizeof(answer), 100, 0);
        if(!strchr(answer,'1')) {
                info("Modem is not registered to the network");
                status=initmodem();
        }
        return status;
}
