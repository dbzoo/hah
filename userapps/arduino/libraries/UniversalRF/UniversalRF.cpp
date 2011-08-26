/* $Id$
  Copyright (c) Brett England, 2011
  Universal RF driver

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include "UniversalRF.h"

UniversalRF::UniversalRF(uint8_t pin) {
  txPin = pin;
  pinMode(pin, OUTPUT);
}

void UniversalRF::setDebug(boolean value) {
  UniversalRF::debug = value;
}

uint8_t UniversalRF::hexToByte(char c)
{
  if (!isxdigit(c)) {
    if(debug) {
      Serial.print("Invalid hex digit ");
      Serial.println(c, BYTE);
    }
    return (uint8_t) 0xff;
  }
  if (isdigit(c))
    return (uint8_t) (c - '0');
  if (isupper(c))
    return (uint8_t) (c - 'A' + 10);
  return (uint8_t) (c - 'a' + 10);
}

// Read a uint8_t in HEX
uint8_t UniversalRF::readByte()
{
  uint8_t c[2];
  uint8_t i;
  for(i=0; i<2; i++) {
    if(*hexPtr == 0) {
      if (debug) 
	Serial.println("Not enough data");
      longjmp(env, 1);
    }
    c[i] = *hexPtr;
    hexPtr++;
  }
  return hexToByte(c[0]) << 4 | hexToByte(c[1]);
}

uint16_t UniversalRF::readWord()
{
  int msb = readByte();
  int lsb = readByte();
  return msb << 8 | lsb;
}

boolean UniversalRF::setupUniversalRF()
{
  uint16_t i;

  // We can get away with this and conserve memory.
  // Its just now the input buffer is destroyed.
  bitStream = (uint8_t *)hexPtr;

  // First uint8_t: bitsPerFrame
  bitsPerFrame = readByte();
  if (bitsPerFrame == 0 || bitsPerFrame > MAXENCBITS || 8 % bitsPerFrame != 0) {
    if(debug) {
      Serial.print("Invalid bitsPerFrame ");
      Serial.println(bitsPerFrame, DEC);
    }
    return false;
  }
  uint8_t enc = 1<<bitsPerFrame;
  framesPerByte = 8 / bitsPerFrame;

  // Used during transmission
  // Pre-calculate to reduce xmit timing issues
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

  burstsToSend = readByte();
  interburstRepeat = readByte();
  interburstDelay = readWord();

  // Adjust for delayMicrosecond() limit of 16383 (Arduino)
  if(interburstDelay > 16383) {
    unsigned long x = interburstRepeat * interburstDelay;
    interBurstRepeat = x/16383 + 1;
    interburstDelay = x / interBurstRepeat;
  }

  frames = readByte();

  int b = frames / framesPerByte;
  if (frames % framesPerByte > 0) // Round up a byte for a partial frame usage
    b++;

  for (i=0; i<b; i++) {
    bitStream[i] = readByte();
  }

  if(debug) {
    Serial.print("Bits per Frame: "); Serial.println(bitsPerFrame, DEC);
    Serial.print("Bursts to send: "); Serial.println(burstsToSend, DEC);
    Serial.print("Frames per Byte: "); Serial.println(framesPerByte,DEC);
    Serial.print("Pulse Encodings: "); Serial.println(enc, DEC);
    Serial.print("Interburst delay repeat: "); Serial.println(interburstRepeat, DEC);
    Serial.print("Interburst delay: "); Serial.println(interburstDelay, DEC);
    Serial.print("Frames: "); Serial.println(frames,DEC);
    Serial.print("StreamBytes: "); Serial.println(b, DEC);
    
    /*
    for(int i=0; i<framesPerByte; i++) {
      Serial.print("mask/shift ");
      Serial.print(i, DEC);
      Serial.print(": ");
      Serial.print(bitMask[i], HEX);
      Serial.print(" ");
      Serial.println(bitShift[i], HEX);
    }
    */    
    for(int i=0; i<enc; i++) {
      Serial.print("Pulse ");
      Serial.print(i,DEC);
      Serial.print(": HI ");
      Serial.print(pulse[i].hi, DEC);
      Serial.print(" ");
      Serial.println(pulse[i].lo, DEC);
    }
  }

  return true;
}

void UniversalRF::xmitBurst()
{
  int8_t j;
  uint8_t i=frames, k=0, idx;
  while(i>0) {
    for(j=framesPerByte; j>=0 && i>0; j--, i--) {
      // Compute Frame index lookup for pulse dimensions
      idx = (bitStream[k] & bitMask[j]) >> bitShift[j];
      digitalWrite(txPin, HIGH);
      delayMicroseconds(pulse[idx].hi);
      digitalWrite(txPin, LOW);
      delayMicroseconds(pulse[idx].lo);
    }
    k++;  // Transmitted all frames for byte, next bitstream byte
  }
}

boolean UniversalRF::transmit(char *rfSequence)
{
  boolean result = false;
  hexPtr = rfSequence;
  uint8_t version;

  if(debug) {
    Serial.println("Transmit RF");
  }
  // Prematurely ran out of data - longjmp here.
  if(setjmp(env) == 1) {
    goto error;
  }
  
  // 1st uint8_t is version (ignored for now)
  version = readByte();

  if (setupUniversalRF()) {
    framesPerByte--; // Adjust for zero based indexing
    cli(); // disable interrupts
    for (uint8_t i=0; i< burstsToSend; i++) {
      xmitBurst();
      for(uint8_t j=interburstRepeat; j>0; j--) {
	delayMicroseconds(interburstDelay);
      }
    }
    sei(); // enable interrupts
    result = true;
  }

error:
  if(debug) {
    Serial.print("RF ");
    Serial.println(result ? "success" : "failure");
  }
  return result;
}
