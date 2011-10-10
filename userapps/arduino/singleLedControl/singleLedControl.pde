#define LEDPIN 13

const int inBufferLen = 255;
uint8_t inPtr = 0;
char inBuffer[inBufferLen+1];

void doCommand() {
  if(strcmp("led on", inBuffer) == 0) {
    digitalWrite(LEDPIN, HIGH);
  } 
  else if(strcmp("led off", inBuffer) == 0) {
    digitalWrite(LEDPIN, LOW);
  }
}

void readSerial() {
  int inByte = Serial.read();
  if(inByte == '\r') { // CR to LF
    inByte = '\n';
  }
  switch(inByte) {
  case '\n':
    if(inPtr > 0) { 
      doCommand(); 
    }
    inPtr = 0;
    break;
  default:
    if(inPtr < inBufferLen) {
      inBuffer[inPtr] = inByte;
      inPtr++;
      inBuffer[inPtr] = '\0';
    }
  }
}

void setup() 
{ 
  Serial.begin(9600); 
} 

void loop()
{
  while(Serial.available()) {
    readSerial();
  }
}

