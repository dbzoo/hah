/*
  File: comm.c
  Desc: This file implements the actual transmission portion
  of the "ok to power me down" message to the remote
  power cycling black box.

  It's been sepatated into a separate file so that
  it may be replaced by any other comm mechanism desired.

  (including none or non serial mode at all)

  $Id: comm.c,v 1.1.1.1 2003/02/28 09:18:55 cmo Exp $
  $Log: comm.c,v $
  Revision 1.1.1.1  2003/02/28 09:18:55  cmo
  Premier import des sources du projet dbw 3g

  Revision 1.1.1.1  2002/01/30 10:16:29  cmo
  Initial import of the project. Was release v1.02-pre12.
  Yet a cvs for bluedsl/etherblue project ...
  CMO 30-01-2002


  Revision 1.1.1.1  2002/01/29 13:25:26  cmo
  Initial import of BLUE sources. Was version v1.02-pre12


  Revision 1.2  2001/06/21 23:07:18  dwmw2
  Initial import to MTD CVS

  Revision 1.1  2001/06/08 22:26:05  vipin
  Split the modbus comm part of the program (that sends the ok to pwr me down
  message) into another file "comm.c"


  
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



/*
  This is the routine that forms and
  sends the "ok to pwr me down" message
  to the remote power cycling "black box".

 */
int do_pwr_dn(int fd, int cycleCnt)
{

    char buf[200];
    
    sprintf(buf, "ok to power me down!\nCount = %i\n", cycleCnt);

    if(write(fd, buf, strlen(buf)) < strlen(buf))
    {
        perror("write error");
        return -1;
    }

    return 0;
}













