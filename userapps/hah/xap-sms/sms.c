/* $Id$
  Copyright (c) Brett England, 2009,2010

  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
 */
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include "xap.h"
#include "modem.h"
#include "putsms.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *interfaceName = "eth0";
const char *inifile = "/etc/xap-livebox.ini";

const char *XAP_CLASS = "sms.message";
#define SEND_INTERVAL 60  // in seconds

// When we receive an "outbound" sms.message report on whether is could be sent or not.
void xap_receipt(char* msg, char* sender, char *sent, char *err)
{
        char buff[XAP_DATA_LEN];

        int len = snprintf(buff, sizeof(buff),
                           "xap-header\n{\nv=12\nhop=1\nuid=%s\n"
                           "class=SMS.Receipt\nsource=%s\n}\n"
                           "Receipt\n{\nmsg=%s\nnum=%s\nsent=%s\nerror=%s\n}\n",
                           xapGetUID(), xapGetSource(), msg, sender, sent, err);
        if(len > sizeof(buff)) {
                err("Buffer overflow");
                return;
        } 
        xapSend(buff);
}


void xap_relay_sms(char* msg, char* sender, char* date, char *time)
{
        char buff[XAP_DATA_LEN];

        int len = snprintf(buff, sizeof(buff),
                           "xap-header\n{\nv=12\nhop=1\nuid=%s\n"
                           "class=%s\nsource=%s\n}\n"
                           "inbound\n{\nmsg=%s\nnum=%s\ntimestamp=%s %s\n}\n",
                           xapGetUID(), XAP_CLASS, xapGetSource(), msg, sender, date, time);

        if(len > sizeof(buff)) {
                err("Buffer overflow");
                return;
        }
	xapSend(buff);
}

void processSerialMsg(char* a_cmd, int len)
{
        // Serial input terminates with \r\n  (17\r\n = length 4)
        // Replace \r with 0
        *(a_cmd+len-1) = 0;

        info("Serial Rx: %s",a_cmd);

        if(strncmp(a_cmd,"+CMTI:",6) == 0) {
                int message_number;
                // Recieved an SMS notification event: +CMTI: "MT",3
                char *p = strchr(a_cmd,',');
                if (p) {
                        message_number = atoi(p + 1);
                } else {
                        // The message begins as expected but has no comma!
                        return;
                }

                struct inbound_sms msg;
                getsms( &msg, message_number );
                xap_relay_sms(msg.ascii, msg.sender, msg.date, msg.time);
        }

}

/// Receive a xap (broadcast) packet and process
void xapSendSMS(void *userData)
{
        char *i_dest = xapGetValue("outbound","num");
        char *i_msg = xapGetValue("outbound","msg");

        switch(putsms(i_msg, i_dest)) {
        case 0:
                xap_receipt(i_msg, i_dest, "no", "Modem not ready");
                break;
        case 1:
                xap_receipt(i_msg, i_dest, "no", "Possible corrupt sms");
                break;
        case 2:
                xap_receipt(i_msg, i_dest, "yes", "");
                break;
        default:
                xap_receipt(i_msg, i_dest, "no", "No idea!");
                break;
        }
}

/// Read data from the MODEM
/// xapSocketListener Callback
void xapSerialAccumulator(int fd, void *userData)
{
#define BUFFER_LEN 250
#define CMD_LEN 1024
        int i_serial_cursor=0;
        int i_serial_data_len;
        char i_serial_buff[BUFFER_LEN+1];
        static char i_serial_cmd[CMD_LEN+1];
        int i_flag=0;
	int i;

        i_serial_data_len = read(fd, i_serial_buff, BUFFER_LEN);
        for(i=0; i < i_serial_data_len; i++) {
                //putchar(i_serial_buff[i]);
                i_serial_cmd[i_serial_cursor++]=i_serial_buff[i];

                if (i_serial_buff[i]=='\r') {
                        i_flag=1;
                } else {
                        if ((i_flag==1) && (i_serial_buff[i]=='\n')) {
                                processSerialMsg(i_serial_cmd, i_serial_cursor);
                                i_serial_cursor=0;
                        } else
                                i_flag=0;
                }
        }
}


int main(int argc, char *argv[])
{
        printf("\nSMS Connector for xAP v12\n");
        printf("Copyright (C) DBzoo, 2009-2010\n\n");

        simpleCommandLine(argc, argv, &interfaceName);
        xapInitFromINI("sms","dbzoo.livebox","SMS","00DD",interfaceName,inifile);

        // SMS inbound from phone
	int fd = setup_serial_port();
        xapAddSocketListener(fd, &xapSerialAccumulator, NULL);

        // SMS outbound to phone
        xAPFilter *f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
        xapAddFilter(&f, "xap-header", "class", XAP_CLASS);
        xapAddFilter(&f, "outbound", "msg", XAP_FILTER_ANY);
        xapAddFilter(&f, "outbound", "num", XAP_FILTER_ANY);
        xapAddFilterAction(&xapSendSMS, f, NULL);

        // Initialize the modem
        if(initmodem()) {
                die("Modem failed to initialize");
        }

        // Do it!
        xapProcess();
}
