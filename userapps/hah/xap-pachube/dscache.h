/* $Id
 */
#ifndef __DSCACHE_H__
#define __DSCACHE_H__

#define DS_TAGSIZE 32

struct datastream {
     int id;
     char tag[DS_TAGSIZE];
     float value;
};

void updateDatastream(int id, char *tag, float value);
char *xmlDatastream();

#endif
