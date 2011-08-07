/* $Id$
  Copyright (c) Brett England, 2011
  Universal RF driver

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include "WProgram.h"
#include <setjmp.h>
#define MAXENCBITS 4

class UniversalRF {
public:
  UniversalRF(uint8_t);
  boolean transmit(char *);
  void setDebug(boolean);

private:
  void xmitBurst();
  uint16_t readWord();
  byte readByte();
  byte hexToByte(char);
  boolean setupUniversalRF();

  struct {
    uint16_t hi;
    uint16_t lo;
  } pulse[1<<MAXENCBITS];

  uint8_t frames;
  uint8_t bitsPerFrame; // valid values: 1,2,4
  uint8_t burstsToSend;
  uint16_t interburstDelay;
  uint8_t interburstRepeat;
  uint8_t bitStream[128]; // 256 HEX chars as bytes = 128
  // Computed values
  uint8_t framesPerByte;
  uint8_t bitMask[8];
  uint8_t bitShift[8];
  char *hexPtr;
  // Used for error handling.
  jmp_buf env;
  uint8_t txPin;
  boolean debug;
};
