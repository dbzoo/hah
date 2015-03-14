/* Copyright (c) Brett England, 2012

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

static int initDurationState(URFContext *ctx, char *pat)
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
  ctx->durStateLen = (frames+1)*2; // +2 frames for start/end leadings.
  ctx->durState = (int *)malloc(ctx->durStateLen * sizeof(int));
  ctx->curState = 0;

  framesPerByte--; // Adjust for ZERO based indexing.
  
  int durStatePos = 1;
  uint8_t idx;

  for(i=frames; i>0;) {
    uint8_t b = readByte();
    debug("Frame %02x", b);

    for(j=framesPerByte; j>=0 && i>0; j--, i--) {
      idx = (b & bitMask[j]) >> bitShift[j];
      debug("Pulse %d", idx);
      ctx->durState[durStatePos++] = pulse[idx].hi;
      ctx->durState[durStatePos++] = -pulse[idx].lo;
    }
  }

  // GAP Pattern at Start and End
  ctx->durState[0] = -interburstRepeat * interburstDelay;
  ctx->durState[durStatePos] = -interburstRepeat * interburstDelay;

  return 1;
}


void *initURFContext(bscEndpoint *e, char *pattern, int state) {
  URFContext *ctx = (URFContext *)calloc(sizeof(URFContext), 1);
  
  if(e) info("%s:%s %s", e->name, e->subaddr, state == BSC_STATE_ON ? "on" : "off");
  info("%s", pattern);
  
  ctx->e = e;
  ctx->state = state;
  if(setjmp(env) == 1) {
    goto error;
  }
  
  if(initDurationState(ctx, pattern)) {
    return (void *)ctx;
  }
  
 error:
  if(ctx->durState) free(ctx->durState);
  free(ctx);
  return NULL;
}

// Compare two values, returning 0 if new is shorter.
// 1 is newval is equal, and 2 if newval is longer.
// Tolerance 20%
static uint8_t pctCompare(int newval, int oldval) {
  newval=abs(newval);
  oldval=abs(oldval);
  if(newval < oldval *.8) {
    return 0;
  } else if(oldval < newval * .8) {
    return 2;
  } else {
    return 1;
  }
}

// The amount of interburst gap that must past before the arduino sketch 
// considers a pulse stream as a candidate for watching.  microseonds
// *MUST* align this value with that in the sketch
#define LEADIN 10000

int URFConsumer(URFContext *ctx, int durstate) {
  debug("URFconsumer got %d need %d - state %d", durstate, ctx->durState[ctx->curState], ctx->curState);
  // First frame from Arduino is a lead-in of at last 10ms (always ok as the 1st frame)
  if((durstate <= -LEADIN && ctx->curState == 0) || pctCompare(durstate, ctx->durState[ctx->curState]) == 1) {
    ctx->curState++;
    debug("OK - %s (%d/%d)", ctx->e->name, ctx->curState, ctx->durStateLen);
    
    if(ctx->curState == ctx->durStateLen-1) { // Adjust for zero based.
      info("MATCH - %s:%s %s", ctx->e->name, ctx->e->subaddr, ctx->state == BSC_STATE_ON ? "on" : "off");
      ctx->curState = 0;
      bscSetState(ctx->e, ctx->state);
      bscSendEvent(ctx->e);
      return 1;
    }
  } else {
    ctx->curState = 0;
    if(durstate <= -LEADIN) { // It could be that what we got was the start of the next.
      ctx->curState++;
      debug("OK - %s (%d/%d)", ctx->e->name, ctx->curState, ctx->durStateLen);
    }
  }
  return 0;
}

void URFReset(URFContext *ctx) {
  ctx->curState = 0;
}
