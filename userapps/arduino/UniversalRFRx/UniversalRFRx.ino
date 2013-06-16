/* $Id$
* 
* This sketch makes the job of catching a RF pulse stream without using a Logic analyzer a little easier.
* It works by looking for two pulse sequences that are identical and separated by a gap of at least 10ms.
*
* Developed using ideas from 
* - IR decoder library by Ken Sherriff
* - Arduinoha project 
*
* brett@dbzoo.com
*/
#include <avr/interrupt.h>

// RF receiver signal in pin
// We don't require an RSSI - just pump all the noise in and we'll deal with it
#define RECV_PIN 4

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
}
rfparams_t;

volatile rfparams_t rfparams;

int currentSignal;
rfparams_t prevrf;

// initialization
void enableRFIn(int recvpin) {
  memset((void *)&rfparams, 0, sizeof(rfparams));
  rfparams.recvpin = recvpin;
  // initialize state machine variables
  rfparams.rcvstate = STATE_IDLE;

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

void resume() {
  rfparams.rcvstate = STATE_IDLE;
  rfparams.rawlen = 0;
}

void dumpRaw(unsigned int rawlen, unsigned int *rawbuf) {
  for (int i = 0; i < rawlen; i++) {
     // yes I know it was unsigned int and I could loose precision.
     // but a real pulse won't be using numbers > 32767
    int val = (*rawbuf)*USECPERTICK;
    Serial.print(i%2 == 0 ? -val : val, DEC);
    if(i < rawlen-1) Serial.print(",");
    rawbuf++;
  }
  Serial.println("");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");  
  enableRFIn(RECV_PIN);
  currentSignal = 0;
}


// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
// Use a tolerance of 20%
int compare(unsigned int oldval, unsigned int newval) {
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
int compareRF() {
  if(prevrf.rawlen != rfparams.rawlen)
    return 0;
  for(int i=0; i<rfparams.rawlen-1; i++) {
    if( compare(prevrf.rawbuf[i], rfparams.rawbuf[i]) != 1) {
      return 0;
    }
  }
  return 1;
}

void loop() {
  if(rfparams.rcvstate == STATE_STOP) {
    if(currentSignal == 0) {
      prevrf.rawlen = rfparams.rawlen;
      for(int i=0;i<rfparams.rawlen;i++) {
        prevrf.rawbuf[i] = rfparams.rawbuf[i];
      }
    } 
    else {
      if(rfparams.rawlen > 3 && compareRF()) {
        dumpRaw(prevrf.rawlen, prevrf.rawbuf);
      }
    }
    currentSignal = !currentSignal;
    resume();     
  }  
}


