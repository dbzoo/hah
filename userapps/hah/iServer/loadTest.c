// Generate a continue stream of xAP packets at 20ms intervals

#include <stdio.h>
#include "xap.h"
char *interfaceName = "eth0";
int main(int argc, char *argv[]) {
	char buf[XAP_DATA_LEN];
	int i = 0;

	simpleCommandLine(argc, argv, &interfaceName);
	xapInit("dbzoo.data.load","FF777700",interfaceName);
	while(1) {
		i++;
		sprintf(buf,"xap-header\n"
                       "{\n"
                       "v=12\n"
                       "hop=1\n"
                       "uid=FF777700\n"
                       "class=data.load\n"
                       "source=dbzoo.data.load\n"
                       "}\n"
                       "body\n"
                       "{\n"
                       "id=%d\n}", i);
		xapSend(buf);
		usleep(20*1000);
	}	
}
