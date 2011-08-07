// Warning only use this for hardware debug!

// see what addresses respond to start condition
//#include <Wprogram.h>
#include <SoftI2cMaster.h>
#include <TwiMaster.h>

// select software or hardware i2c
#define USE_SOFT_I2C 1

#if USE_SOFT_I2C

// pins for software i2c
#define SCL_PIN 2
#define SDA_PIN 3

// An instance of class for software master
SoftI2cMaster rtc;

#else // USE_SOFT_I2C

// Pins for hardware i2c
// connect SCL to Arduino 168/328 analog pin 5
// connect SDA to Arduino 168/328 analog pin 4

// Instance of class for hardware master
TwiMaster rtc;

#endif  // USE_SOFT_I2C


void setup(void) 
{ 
  Serial.begin(9600);
#if USE_SOFT_I2C
  // set i2c pins pullups enabled by default
  rtc.init(SCL_PIN, SDA_PIN);
#else
  // true to enable pullups
  rtc.init(true);
#endif  
   uint8_t add = 0;
 
  // try read
 do {
   if (rtc.start(add | I2C_READ)) {
      Serial.print("Add read: ");
      Serial.println(add, HEX);
      rtc.read(true);
    }
    rtc.stop();
    add += 2;
  } while (add);
  
  // try write
 add = 0;
 do {
   if (rtc.start(add | I2C_WRITE)) {
      Serial.print("Add write: ");
      Serial.println(add, HEX);
    }
    rtc.stop();
    add += 2;
  } while (add);
    
  Serial.println("Done");
}
void loop(void){}
