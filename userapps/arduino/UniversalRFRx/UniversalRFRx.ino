/* $Id$
 * Universal RF Rx
 * 
 * Code heavily borrowed from: http://arduinoha.googlecode.com
*/

// Comment all for PRODUCTION.
#define ShowErrors
#define ShowReceivedCommands
#define ShowReceivedPulses

// DEF
#define SerialBaudrate 115200
#define PRESCALER256

// The datapin defines the pin which is wired-up to the Data-output-pin of the receiver-module.
unsigned int DATAPIN = 3;  // The pin number of the data signal : (Digital 3 = chip pin 5 - PD3

// If the receiver-module has a RSSI-output-pin, this pin can be wired-up to only process strongly received signals.
// If the receiver-module does not have a RSSI-output-pin, the data-output-pin can be used as a trigger on every signal (even noise). 
unsigned int RSSIPIN    = 3; // The pin number of the RSSI signal
unsigned int RSSIIRQNR  = 1;  // The irq number of the RSSI pin - 0 (PD2) and 1 (PD3)


// The timings of the pulses received from the receiver-module will be stored in this Circul/Ring-buffer. 
// The reason for this is that these pulses can be "offline" processed.
#define ReceivedPulsesCircularBufferSize 400
int receivedpulsesCircularBuffer[ReceivedPulsesCircularBufferSize];

// The index number of the first pulse which will be read from the buffer.
volatile unsigned short receivedpulsesCircularBuffer_readpos = 0; 

// The index number of the position which will be used to store the next received pulse.
volatile unsigned short receivedpulsesCircularBuffer_writepos = 0; 

volatile unsigned int prevTime; // ICR1 (timetick) value  of the start of the previous Pulse

enum SendReceiveStateEnum
{
  uninitialized,
  waitingforrssitrigger,
  listeningforpulses,
};

volatile SendReceiveStateEnum sendreceivestate = uninitialized;

void ShowPulse(int durState)
{
// If debugging make it ASCII so it can be read.
#ifdef ShowErrors
  Serial.print("[");
  if (durState < 0 ) Serial.print("1"); else Serial.print("0");
  Serial.print(",");
  Serial.print(abs(durState), DEC);
  Serial.print("] ");
#else
  // pump binary data for faster transfer rates and lower overhead.
  Serial.write((const uint8_t *)&durState, sizeof(int));
#endif
}

// This function will store a received pulse in the Circular buffer
void StoreReceivedPulseInBuffer(unsigned int duration, byte state)
{
    // Store pulse in receivedpulsesCircularBuffer
    receivedpulsesCircularBuffer[receivedpulsesCircularBuffer_writepos] = state == LOW ? duration : -duration;
   
    // When at the end of the buffer start from beginning of buffer
    if (++receivedpulsesCircularBuffer_writepos >= ReceivedPulsesCircularBufferSize) {
      receivedpulsesCircularBuffer_writepos = 0;
    }

    #ifdef ShowErrors    
      // Check overflow
      if (receivedpulsesCircularBuffer_writepos==receivedpulsesCircularBuffer_readpos)
      { // Overflow circular buffer
        Serial.print("Overflow");
      }
    #endif
}

// This Interrupt Service Routine will be called upon each change of data on DATAPIN when listeningforpulses.
ISR(TIMER1_CAPT_vect)
{ 
  // Store the value of the timer
  unsigned int newTime = TCNT1 ; //ICR1; 

  // We are only interested in pulses when "listeningforpulses"
  if (sendreceivestate==listeningforpulses) 
  {
    byte curstate ;

    // Were we waiting for a "falling edge interrupt trigger?
    if ( (TCCR1B & (1<<ICES1)) == 0)
    { // ISR was triggerd by falling edge
        // set the ICES bit, so next INT is on rising edge
        TCCR1B |= (1<<ICES1);
        curstate = LOW;
    }
    else
    { // ISR was triggered by rising edge
        //clear the ICES1 bit so next INT is on falling edge
        TCCR1B &= (~(1<<ICES1));      
        curstate = HIGH;
    } 

    // Calculate the duration of the pulse
    unsigned int time = (newTime - prevTime);
    
    // Did the timer overflow?
    if (newTime<prevTime)
    {
      // Calculate the duration of the pulse taking into account the overflowing of the timer
      time =  ((!prevTime) + 1) + newTime;    
    }

    // Store the duration of the received pulse    
    StoreReceivedPulseInBuffer(time, !curstate);
  } ;
  
  prevTime = newTime;  
}

// This Interrupt Service Routine will be called upon an timer-overflow. This will happen every few seconds when receiving.
// It will also happen for each sent pulse after the duration.
ISR(TIMER1_OVF_vect) 
{
  if(sendreceivestate == listeningforpulses) {
      // Disable timer-overflow interrupt
      TIMSK1 = 0 ;
      
      // Add some pulses to flush decoders
      StoreReceivedPulseInBuffer(65535-prevTime,HIGH);
      StoreReceivedPulseInBuffer(65535-prevTime,LOW);

      // Timer has overflowed, start waiting again
      sendreceivestate=waitingforrssitrigger;
  }
}

// This function will be triggered by a high RSSI signal
void rssiPinTriggered(void)
{
  if (sendreceivestate == waitingforrssitrigger)
  {
    sendreceivestate = listeningforpulses;
    prevTime = 0;//ICR1;
     // reset the counter as close as possible when the rssi pin is triggered (to achieve accuracy)
    TCNT1 = 0;
    TIMSK1=1<<ICIE1 /* enable capture interrupt */ | 1<<TOIE1 /* enable timer overflow interrupt */; 
    TIFR1 |= 1 << TOV1; //clear the "pending overflow interupt" flag
    
    //clear the ICES1 bit so next INT is on falling edge
    TCCR1B&=(~(1<<ICES1));      

    #ifdef ShowReceivedPulses
      Serial.println("RSSI");
    #endif
  }
}

void loop()
{
  // Is there still a pulse in the receivedpulses Circular buffer?
  while (receivedpulsesCircularBuffer_readpos != receivedpulsesCircularBuffer_writepos)
  { // yes
    int durState = receivedpulsesCircularBuffer[receivedpulsesCircularBuffer_readpos];    
    if (++receivedpulsesCircularBuffer_readpos >= ReceivedPulsesCircularBufferSize) 
    {
      receivedpulsesCircularBuffer_readpos = 0;
    }
    ShowPulse(durState);
  }
}

void setup()
{  
  // Initialize the serial-output
  Serial.begin(SerialBaudrate);
    
  // Initialize the pins
  pinMode(DATAPIN, INPUT);
  pinMode(RSSIPIN, INPUT);

  sendreceivestate = waitingforrssitrigger;
  
  TCCR1A=0;   // Normal mode (Timer1)
  #ifdef PRESCALER256
    TCCR1B = 0<<ICES1 /* Input Capture Edge Select */ | 1<<CS12 /* prescaler */ | 0<<CS11 /* prescaler */  | 0<<CS10 /* prescaler */ | 1<<ICNC1 /* Capture Noice Canceler */;  
  #else
    TCCR1B = 0<<ICES1 /* Input Capture Edge Select */ | 0<<CS12 /* prescaler */ | 1<<CS11 /* prescaler */  | 1<<CS10 /* prescaler */ | 1<<ICNC1 /* Capture Noice Canceler */;  
  #endif
  TIMSK1 = 0<<ICIE1 /* Timer/Counter 1, Input Capture Interrupt disabled */ ;  

  attachInterrupt(RSSIIRQNR, rssiPinTriggered , RISING);
}
 

