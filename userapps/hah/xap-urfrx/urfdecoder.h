/* Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifndef _URFDECODER_H
#define _URFDECODER_H

#include <stdint.h>
#include "bsc.h"

#define MAXENCBITS 4

typedef struct _URFContext {
  bscEndpoint *e;
  int state;

  int *durState;
  uint16_t durStateLen;
  uint16_t curState;
} URFContext;

void *initURFContext(bscEndpoint *e, char *pattern, int state);
int URFConsumer(URFContext *ctx, int durstate);
void URFReset(URFContext *ctx);

#endif
