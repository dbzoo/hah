#ifndef _MODEM_H
#define _MODEM_H

#define TIME_LEN 9 /* xx-xx-xx */
#define DATE_LEN TIME_LEN 

int put_command(char *cmd, int cmd_len, char *answer, int max, int timeout, char *exp_end);
int initmodem();
int setup_serial_port();
int checkmodem();

extern int g_serial_fd;
extern const char *inifile;

struct inbound_sms {
	 char sender[31];
	 char name[64];
	 char date[DATE_LEN];
	 char time[TIME_LEN];
	 char ascii[500];
	 int userdatalength;
}; 

#endif
