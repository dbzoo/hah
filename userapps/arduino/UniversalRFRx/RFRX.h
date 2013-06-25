/** $Id$
 * Version 0.1, June, 2013
 * Copyright 2013 Brett England
 */
#ifndef RFRX
#define RFRX

#include <Arduino.h> // Arduino 1.0
#include <avr/interrupt.h>

#ifdef F_CPU
#define SYSCLOCK F_CPU // main Arduino clock
#else
#define SYSCLOCK 16000000 // main Arduino clock
#endif

#define USECPERTICK 16 // microseconds per clock interrupt tick
// On a 16bit timer gives 16*65536/1000=1048ms rollover

#define MARK 1
#define SPACE 0

// receiver states
#define STATE_IDLE 2
#define STATE_MARK 3
#define STATE_SPACE 4
#define STATE_STOP 5

#define _GAP 10000 // Minimum gap between transmissions (uS)
#define GAP_TICKS (_GAP/USECPERTICK)

#define RAWBUF 200 // Length of raw duration buffer

// information for the interrupt handler
typedef struct {
  uint8_t recvpin; // pin for IR data from detector
  uint8_t rcvstate; // state machine
  unsigned int timer; // state timer, counts ticks.
  unsigned int rawbuf[RAWBUF]; // raw data
  uint8_t rawlen; // counter of entries in rawbuf
  int currentSignal;
}
rfparams_t;

class RFResults {
public:
  unsigned int buf[RAWBUF]; // raw data
  uint8_t len; // counter of entries in rawbuf   
};

class RFrecv
{
public:
  RFrecv(int pin);
  void enableRFIn();
  void resume();
  int match(RFResults *results);
private:
  int compare(unsigned int oldval, unsigned int newval);
  int compareRF(RFResults *results);
};

#endif

