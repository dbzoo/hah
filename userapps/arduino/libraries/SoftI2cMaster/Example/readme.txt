I wrote these libraries for use with simple slave devices like real
time clocks, EEPROM, ADCs, and simple sensors.

The interrupt driven Arduino Wire library has a number of disadvantages
for these simple devices where the Arduino is the master.  The Wire
library has limited transfer size due to buffer restrictions, is hard
to debug since it is interrupt driven, and only supports the TWI
hardware pins.

There are two libraries, SoftI2cMaster and TwiMaster.  They are both
derived from the TwoWireBase base class.  This allows applications to
use either library by only changing the init() call in setup().

SoftI2cMaster is a software i2c library that can use any two Arduino
pins for SCL and SDA.  It has an i2c clock rate of about 65 kHz.

TwiMaster uses the ATmega TWI hardware so you must use the hardware
SCL and SDA pins.  It is setup with a i2c clock rate of 400 kHz.

To install the these libraries, copy the folders SoftI2cMaster and
TwiMaster to the Arduino libraries directory.

I have not yet produced good documentation so I am including an
application, softDS1307.pde to illustrate the use of these libraries.
You must connect a DS1307 real time clock to the correct pins to run
this application.

The functions setup(), readDS1307(), and writeDS1307 in softDS1307.pde
show how to use these libraries.  Please read these functions.

softDS1307.pde actually can use either library by setting the value of
the macro USE_SOFT_I2C.

If USE_SOFT_I2C is nonzero, connect the DS1307 SCL to digital pin 2 and
SDA to digital pin 3.

If USE_SOFT_I2C is zero, connect the DS1307 to the hardware SDA and
SCL pins.  On a 168/328 Arduino SDA is analog pin 4 and SCL is analog
pin 5.

softDS1307.pde prints USA style dates - mm/dd/yyyy.

The menu for softDS1307.pde is:

Options are:
(0) Display date and time
(1) Set time
(2) Set date
(3) Set Control
(4) Dump all
(5) Set NV RAM

The first three are obvious.

Set control - sets the register that controls the operation of the
DS1307's SQW/OUT pin.  See comments for the setControl() function
in softDS1307.pde.

Dump all - dumps all 64 locations in the DS1307.

Set NV RAM - sets locations 0X08 through 0X3F to the value you enter.


Other Sketches

i2cScanAddress.pde - See which addresses respond to start condition.

Use the i2cScopeTest.pde sketch to look at soft i2c signals.  SCL is
on digital pin 7 and SDA is on digital pin 8.  Enter zero for write
mode. The byte 0X55 is sent in loop with a 1 ms delay between bytes.
The i2c clock rate is about 65 kHz.

Enter nonzero to look at read clock. The clock rate is about the same
as write.

SoftI2cMaster seems to work with delays removed from loops at 85 kHz but
SCL is not a uniform square wave.

