/* $Id$
*/

#ifndef INI_H
#define INI_H

#ifdef DEBUG
const char inifile[] = "./xap-livebox.ini";
#else
const char inifile[] = "/etc/xap-livebox.ini";
#endif

void setupXAPini();
void parseini();

#endif
