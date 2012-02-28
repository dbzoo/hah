/* $Id$
   Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#ifndef _RFRX_H
#define _RFRX_H

#include <stdint.h>
#include "utlist.h"

typedef struct _RFdecoder {
   void *ctx;
   int (*signalConsumer)(void *ctx, int durState);
  struct _RFdecoder *next;
} RFdecoder;

#endif
