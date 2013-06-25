/** $Id$
* Version 0.1, June, 2013
* Copyright 2013 Brett England
*/
#include "RFRX.h"

volatile rfparams_t rfparams;

// initialization
RFrecv::RFrecv(int recvpin) {
  rfparams.recvpin = recvpin;
  rfparams.rcvstate = STATE_STOP;
  rfparams.currentSignal = 0;
}

void RFrecv::enableRFIn() {
  cli();
  // setup pulse clock timer interrupt
  // http://www.avrbeginners.net/architecture/timers/timers.html
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);  // CTC, No Prescaler
  OCR1A = SYSCLOCK * USECPERTICK / 1000000;
  TCNT1 = 0;
  //Timer2 Overflow Interrupt Enable
  TIMSK1 = _BV(OCIE1A);
  sei(); // enable interrupts
  
  // set pin modes  
  rfparams.rcvstate = STATE_IDLE;  
  rfparams.rawlen = 0;
  
  pinMode(rfparams.recvpin, INPUT);
}

// TIMER2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 16 microseconds.
// rawlen counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
// As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts
ISR(TIMER1_COMPA_vect)
{
  uint8_t irdata = (uint8_t)digitalRead(rfparams.recvpin);

  rfparams.timer++; // One more tick
  if (rfparams.rawlen >= RAWBUF) {
    // Buffer overflow
    rfparams.rawlen = 0;
    //rfparams.rcvstate = STATE_STOP;
  }
  switch(rfparams.rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == MARK) {
      if (rfparams.timer < GAP_TICKS) {
        // Not big enough to be a gap.
        rfparams.timer = 0;
      }
      else {
        // gap just ended, record duration and start recording transmission
        rfparams.rawlen = 0;
        rfparams.rawbuf[rfparams.rawlen++] = rfparams.timer;
        rfparams.timer = 0;
        rfparams.rcvstate = STATE_MARK;
      }
    }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == SPACE) { // MARK ended, record time
      rfparams.rawbuf[rfparams.rawlen++] = rfparams.timer;
      rfparams.timer = 0;
      rfparams.rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == MARK) { // SPACE just ended, record it
      rfparams.rawbuf[rfparams.rawlen++] = rfparams.timer;
      rfparams.timer = 0;
      rfparams.rcvstate = STATE_MARK;
    }
    else { // SPACE
      if (rfparams.timer > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        rfparams.rawbuf[rfparams.rawlen++] = rfparams.timer;
        rfparams.rcvstate = STATE_STOP;
      }
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == MARK) { // reset gap timer
      rfparams.timer = 0;
    }
    break;
  }
}

void RFrecv::resume() {
  rfparams.rcvstate = STATE_IDLE;
  rfparams.rawlen = 0; 
  rfparams.currentSignal = !rfparams.currentSignal;
}

// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
// Use a tolerance of 20%
int RFrecv::compare(unsigned int oldval, unsigned int newval) {
  if (newval < oldval * .8) {
    return 0;
  }
  else if (oldval < newval * .8) {
    return 2;
  }
  else {
    return 1;
  }
}

// Compare two signals
// 1 Match, 0 otherwise
int RFrecv::compareRF(RFResults *prevrf) {
  if(prevrf->len != rfparams.rawlen)
    return 0;
  for(int i=0; i<rfparams.rawlen-1; i++) {
    if( compare(prevrf->buf[i], rfparams.rawbuf[i]) != 1) {
      return 0;
    }
  }
  return 1;
}

int RFrecv::match(RFResults *prevrf) {
  if(rfparams.rcvstate == STATE_STOP) {
    if(rfparams.currentSignal == 0) {
      prevrf->len = rfparams.rawlen;
      for(int i=0;i<rfparams.rawlen;i++) {
        prevrf->buf[i] = rfparams.rawbuf[i];
      }
    } 
    else {
      if(rfparams.rawlen > 3 && compareRF(prevrf)) {
        return 1;
      }
    }    
    resume();
  }  
  return 0;
}


