/* $Id: ini.h 33 2009-11-10 09:56:39Z brett $
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
