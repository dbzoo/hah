/* $Id$
   Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifndef _URFDECODER_H
#define _URFDECODER_H

#include <stdint.h>

#define MAXENCBITS 4

struct URFContext {
  char *xAPTarget;
  char *bscState;

  int *durState;
  uint16_t durStateLen;
  uint16_t curState;

};

void *initURFContext(char *pattern, char *target, char *state);
int URFConsumer(void *ctxp, int durstate);

#endif
