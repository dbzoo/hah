#include <stdio.h>
#include "modem.h"
#include "log.h"

int putsms(char *msg, char *num)
{
        char command[50];
        char command2[500];
        int clen, clen2;
        int err_code;
        int retries;
        char answer[100];


        info("Dest <%s>", num);
        info("Msg <%s>", msg);

        clen = sprintf(command,"AT+CMGS=\"%s\"\r", num);
        clen2 = sprintf(command2, "%s\x1A", msg);

        for(err_code=0, retries=0; err_code<2 && retries<3; retries++) {
                if( put_command(command, clen, answer,sizeof(answer), 50,"\r\n> ")
                                && put_command(command2, clen2, answer, sizeof(answer), 1000, 0)
                                && strstr(answer,"OK") ) {
                        err_code = 2;
                } else {
                        if(checkmodem() == -1) {
                                err_code = 0;
                                info("Resending last SMS");
                        } else if (err_code == 0) {
                                info("Possible corrupted sms. Trying again.");
                                err_code = 1;
                        } else {
                                warning("No idea?!  Dropping sms");
                                err_code = 3;
                        }
                }
        }
        if(err_code == 0) {
                warning("re-inited and re-tried %d times without success", retries);
        }
        return err_code;
}
