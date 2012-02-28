/* $Id$
   Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "log.h"
#include "urfdecoder.h"

static char *hexPtr;
static jmp_buf env; // Handling Fatal OOB issues

static uint8_t hexToByte(char c) {
  if(!isxdigit(c)) {
    err("Invalid hex digit %x", c);
    return (uint8_t) 0xff;
  }
  if (isdigit(c))
    return (uint8_t) (c - '0');
  if (isupper(c))
    return (uint8_t) (c - 'A' + 10);
  return (uint8_t) (c - 'a' + 10);
}

// Read a uint8_t in HEX
static uint8_t readByte()
{
  uint8_t c[2];
  uint8_t i;
  for(i=0; i<2; i++) {
    if(*hexPtr == 0) {
      crit("Not enough data");
      longjmp(env, 1); // This is REALLY BAD.
    }
    c[i] = *hexPtr;
    hexPtr++;
  }
  return hexToByte(c[0]) << 4 | hexToByte(c[1]);
}

static uint16_t readWord()
{
  int msb = readByte();
  int lsb = readByte();
  return msb << 8 | lsb;
}

static int initDurationState(struct URFContext *ctx, char *pat)
{
  struct {
    uint16_t hi;
    uint16_t lo;
  } pulse[1<<MAXENCBITS];

  uint8_t bitMask[8];
  uint8_t bitShift[8];
  int i, j;

  hexPtr = pat; // Init our global.

  // 1st uint8_t is version (ignored for now)
  readByte();

  uint8_t bitsPerFrame = readByte();
  if (bitsPerFrame == 0 || bitsPerFrame > MAXENCBITS || 8 % bitsPerFrame != 0) {
    err("Invalid bitsPerFrame %d", bitsPerFrame);
    return 0;
  }
  uint8_t enc = 1<<bitsPerFrame;
  uint8_t framesPerByte = 8 / bitsPerFrame;

  bitMask[0] = enc-1; // 1bit=1, 2bit=3, 4bit=15
  bitShift[0] = 0;
  for(i=1; i<framesPerByte; i++) {
    bitMask[i] = bitMask[i-1] << bitsPerFrame;
    bitShift[i] = bitShift[i-1] + bitsPerFrame;
  }

  // Populate lookup table of pulse timings
  for (i=0; i< enc; i++) {
    pulse[i].hi = readWord();
    pulse[i].lo = readWord();
  }

  info("bursts to send %d",readByte()); // burstsToSend
  uint8_t interburstRepeat = readByte();
  uint16_t interburstDelay = readWord();
  uint8_t frames = readByte();

  if(getLoglevel() >= LOG_INFO) {
    info("Bits per Frame: %d", bitsPerFrame);
    info("Frames per Byte: %d", framesPerByte);
    info("Pulse Encodings: %d", enc);
    info("Interburst delay repeat: %d", interburstRepeat);
    info("Interburst delay: %d", interburstDelay);
    info("Frames: %d", frames);
    
    for(i=0; i<framesPerByte; i++) {
      debug("mask/shift %d : %02x %02x", i, bitMask[i], bitShift[i]);
    }

    for(i=0; i<enc; i++) {
      info("Pulse %d: %d HI %d LO", i, pulse[i].hi, pulse[i].lo);
    }
  }

  // Setup Duration/State encoding
  ctx->durStateLen = (frames + 1) * 2; // +1 special intergap frame
  ctx->durState = (int *)malloc(ctx->durStateLen * sizeof(int));
  ctx->curState = 0;

  framesPerByte--; // Adjust for ZERO based indexing.
  
  int durStatePos = 2; // Frames 0 and 1 will be added last.
  uint8_t idx;

  for(i=frames; i>0;) {
    uint8_t b = readByte();
    debug("Frame %02x", b);

    for(j=framesPerByte; j>=0 && i>0; j--, i--) {
      idx = (b & bitMask[j]) >> bitShift[j];
      debug("Pulse %d", idx);
      ctx->durState[durStatePos++] = -pulse[idx].hi;
      ctx->durState[durStatePos++] = pulse[idx].lo;
    }
  }

  // Pattern GAP Pattern GAP Pattern
  //
  // The last FRAME of the 1st Pattern forms part of the interGAP
  // Setup GAP synchronization PULSE at the start.
  // Use timing of last high pulse + intergap

  ctx->durState[0] = -pulse[idx].hi;
  ctx->durState[1] = interburstRepeat * interburstDelay;

  return 1;
}


void *initURFContext(char *pattern, char *target, char *state) {
    struct URFContext *ctx = (struct URFContext *)malloc(sizeof(struct URFContext));
    
    info("%s %s %s", pattern, target, state);

    ctx->xAPTarget = strdup(target);
    ctx->bscState = strdup(state);

    if(setjmp(env) == 1) {
      goto error;
    }

    if(initDurationState(ctx, pattern)) {
      return (void *)ctx;
    }

 error:
    free(ctx->xAPTarget);
    free(ctx->bscState);
    free(ctx->durState);
    free(ctx);
    return NULL;
}

static uint8_t withinDeviation(int durState, int expDurState, unsigned int deviation) {
  return abs(durState - expDurState) <= deviation ? 1 : 0;
}

int URFConsumer(void *ctxp, int durstate) {
  int ret;
  struct URFContext *ctx = (struct URFContext *)ctxp;
  
  if(withinDeviation(durstate, ctx->durState[ctx->curState], 10)) { // 10us FUZZY
    //if(durstate == ctx->durState[ctx->curState]) {
    ctx->curState++;
    //debug("%s - %s -> FSM %d", ctx->xAPTarget, ctx->bscState, ctx->curState);
    if(ctx->curState == ctx->durStateLen) {
      printf("MATCH - %s - %s\n", ctx->xAPTarget, ctx->bscState);
      ctx->curState = 0;
      ret = 1;
    }
  } else {
    ctx->curState = 0;
  }
  return ret;
}
