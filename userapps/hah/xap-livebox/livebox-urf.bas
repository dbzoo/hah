'********************************************************************
'* Livebox HAH firmware
'* Language:       BASCOM-AVR 2.0.3
'*
'* $Id$
'********************************************************************


$regfile = "m328pdef.dat"

' Firmware revision
Const Fwmajor = 2
Const Fwminor = 2

' Allow interactive debug mode - increases code size.
Const Allow_interactive = 1

' production or Arduino dev
Const Production = 1

' Compiling for the custom SERIAL PCB
#if Production

$crystal = 7372800                                          ' External XTAL
$baud = 115200

#else

' Compiling for the Arduino board
$crystal = 16000000
$baud = 9600

#endif
'+--------------+  Top of sram 0x045F
'|              |
'|   Hardware   |
'|     stack    |
'+--------------+
'|   Software   |
'|     stack    |
'+--------------+
'|    Frame     |
'+--------------+
'|   24 Byte    | <- subtracted from declared $FRAMESIZE
'|Frame Reserved|
'|--------------|
'|              |
'| Unused SRAM  |
'|              |
'+--------------+
'|              |
'|  User Dimmed |
'|   Variables  |
'|              |
'+--------------+  Bottom of sram 0x0060

$hwstack = 64                                               ' 2 bytes per call/gosub + upto 32 bytes for interrupts
$swstack = 32                                               ' 2 bytes per local variable + 2 bytes per parameter passed
$framesize = 16                                             ' Default

'STARTUP
Declare Sub Bootup_wait()
'COMMAND PROCESSOR
Declare Sub Printprompt()
Declare Sub Docommand()
Declare Sub Getinput(byval Pbbyte As Byte)
'OTHER COMMANDS
Declare Sub Help()
Declare Sub Reboot
Declare Sub Lcddisplay(pos As Byte)
Declare Sub Port_status
'RELAY
Declare Sub Poweron(byval Chnl As Byte)
Declare Sub Poweroff(byval Chnl As Byte)
Declare Sub Reset_relays()
Declare Sub Relay_status()
'1-WIRE
Declare Sub 1wire_status()
Declare Sub 1wire_setup()
Declare Sub Convallt                                        ' Convert T on ALL sensors
Declare Sub Meas_to_cel(id As Byte , Offset As Byte)
Declare Sub 1wire_monitor
Declare Sub Disp_temp(cnt As Byte , Offset As Byte)
'I2C
Declare Sub I2cmgmt(pos As Byte)
Declare Sub I2c_monitor
'RF sub-system
Declare Sub Readbyte
Declare Sub Readword
Declare Sub Readlong
Declare Sub Setupurf
Declare Sub Xmitrf
Declare Sub Universalrfv1(pos As Byte)
Declare Sub Universalrf(pos As Byte)

' I want a timer that resolves to 1sec, so the first step is to divide down
' the input clock .  Input clock to divide by 256, so now each clock pulse is
' worth (7.3728/256)=28.8uS.

' The trick is to preload the counter with a value that will cause it to count
' for 750mS. Each time the counter expires, you can force the new number back
' into the timer register. We need 1/.0000288 or 34722 counts. However, the
' counter expires at 65536, so the correct number to preload is 65536-34722 or
' 30813 This way, the timer will count up 34722 counts (1s) and expire.

Const 1sec = 65536 - 1 /(_xtal / 256 / 1000000000)

'#if Production
'Const 1sec = 30813                                          ' @ 7.3728Mhz
'#else
'Const 1sec = 1536                                           ' @ 4Mhz
'Const 1sec = 49536                                          ' @ 16Mhz
'#endif

' Temperator sensor configuration
Const Ds18b20_conf_reg = 4

' constant to convert the fraction bits to cel*(10^-4)
Const Ds18x20_fracconv = 625

' DS18x20 ROM ID
Const Ds18s20_id = &H10
Const Ds18b20_id = &H28
Const Ds1822_id = &H22

' COMMANDS
Const Ds18x20_convert_t = &H44                              ' Tell device to take a temp reading and put in scratch
Const Ds18x20_copy = &H48                                   ' Copy eeprom
Const Ds18x20_read = &HBE                                   ' Read eeprom
Const Ds18x20_write = &H4E                                  ' write to eeprom
Const Ds18x20_recall = &HB8                                 ' reload from last known
Const Ds18x20_read_power_supply = &HB4                      ' determine if device needs parasite power

' If you change this number you must setup the PORT mappings
' in the initalize section.  Also the internal relays are always mapped to the
' first channels IDs.

' PORT B
'   0 : Relay 1
'   1 : Relay 2
'   2 : Relay 3
'   3 : Relay 4
'   4 : 1-Wire
'   5 : RF
'   6 : SV1-(4,5)
'   7 : SV1-(3,2)
Const Relays = 4
Config Portb.0 = Output , Portb.1 = Output , Portb.2 = Output , Portb.3 = Output
Config 1wire = Portb.4                                      ' Only 1 pin can be configure as 1wire
Awirep Alias Pinb.5
Awire Alias Portb.5
Config Awirep = Output

' PORT C
'   0 : DB4 (LCD)
'   1 : DB5 (LCD)
'   2 : DB6 (LCD)
'   3 : DB7 (LCD)
'   4 : E   (LCD)
'   5 : RS  (LCD)
'   6 : n/c
'   7 : n/c
Config Lcdpin = Pin , Db4 = Portc.2 , Db5 = Portc.3 , Db6 = Portc.4 , Db7 = Portc.5 , E = Portc.1 , Rs = Portc.0
Config Lcd = 16 * 1a                                        'configure lcd screen

' PORT D
'   0 : RXD
'   1 : TXD
'   2 : Input 1
'   3 : Input 2
'   4 : Input 3
'   5 : Input 4
'   6 : I2C
'   7 : I2C
Config Pind.2 = Input , Pind.3 = Input , Pind.4 = Input , Pind.5 = Input
Config Sda = Portd.6
Config Scl = Portd.7

Config Serialin = Buffered , Size = 30

' Timer used to poll 1-wire devices
Config Timer1 = Timer , Prescale = 256
On Timer1 Timer1_int
Dim Time1_count As Bit                                      ' Time tick 1 or 0
Dim Time1_ok As Bit                                         ' Set when an interrupt occurs

' DEBUG
#if Allow_interactive
Dim Interactive As Bit
#endif

' Input sensors
Dim Iport As Byte                                           ' Previous input port value
Dim Portvalue As Byte                                       ' Current Input port value (TEMP)

' Relays
Dim Rport(relays) As Byte

' Serial input
Dim Gbinp As Byte                                           ' holds user input
Const Cpcinput_len = 255                                    ' max. length of user-Input
Dim Gspcinput As String * Cpcinput_len                      ' holds user-input
Dim Gspcinp(cpcinput_len) As Byte At Gspcinput Overlay
Dim Gbpcinputpointer As Byte                                ' string-pointer during user-input

' 1-wire devices
Const Max1wire = 31
' Up to 31 each having an 8 byte ROMID
Dim Dsid(248) As Byte                                       ' Dallas ID 64bit inc. CRC
Dim Dsvalue(max1wire) As Word                               ' Value of each sensor
Dim Dssign As Long
Dim Cnt1wire As Byte                                        ' Number of 1-wire devices found

' I2C PPE devices
Const Max_ppe = 8
Dim Ppe_count As Byte
Ppe_count = 0
Dim Ppe_addr(8)as Byte
Dim Ppe_val(8) As Byte                                      ' Last read value value

' Bootup wiggler
Const Wiggle_len = 4
Dim Wiggle As String * Wiggle_len
Dim Wigglec(wiggle_len) As Byte At Wiggle Overlay

' Bit positions that are reset once a report is issued.
Dim Report As Byte
Const Repinput = 0
Const Rep1wire = 1
Const Repppe = 2

' Temporary variables
Dim Pos As Byte
Dim I As Byte
Dim J As Byte
Dim K As Byte
Dim L As Byte
Dim Command As String * 4
Dim Commando(4) As Byte At Command Overlay
Dim Bp As Byte
Dim D As Byte
Dim B As Byte
Dim Iw As Word


' Variables used by the INTERRUPT routine - Don't use these for general purpose
' as their values will unexpectedly change.
Dim Ii As Byte
Dim Ij As Byte
Dim Ik As Byte
Dim Bi As Word
Dim Di As Byte
Dim Imeas As Word
Dim Temp As Integer
Dim Sc(9) As Byte                                           ' 1-wire scratch pad
'Sc(1) 'Temperature LSB
'Sc(2) 'Temperature MSB
'Sc(3) 'TH/user byte 1 also SRAM
'Sc(4) 'TL/user byte 2 also SRAM
'Sc(5) 'config  also SRAM x R1 R0 1 1 1 1 1 - the r1 r0 are config for resolution - write FF to byte for 12 bit
'Sc(6) 'res
'Sc(7) 'res
'Sc(8) 'res
'Sc(9) '8 CRC

'RF Control data
Const Maxencbits = 4
Const Maxbitstream = 64
Dim Pulsehi(16) As Word                                     ' (1<<maxencbits)
Dim Pulselo(16) As Word
Dim Frames As Byte
Dim Bitsperframe As Byte
Dim Burststosend As Byte
Dim Interburstrepeat As Byte
Dim Interburstdelay As Word
Dim Bitstream(maxbitstream) As Byte

' RF working data
Dim Framesperbyte As Byte
Dim Bitmask(8) As Byte
Dim Bitshift(8) As Byte
Dim Enc As Byte
Dim Streambytes As Byte
Dim Hbyte As Byte
Dim Hword As Word
Dim Hlong As Long
Dim Gppos As Byte
Dim Hexstring As String * 2
Dim Hexbyte(2) As Byte At Hexstring Overlay
Dim Rferr As Bit

'*****************************************************************************
'#### MAIN ####

Enable Interrupts
Report = 0
#if Allow_interactive
Reset Interactive                                           ' We are not in interactive by default.
#endif
Reset_relays
1wire_setup
Bootup_wait                                                 ' Wait for +++

Set Portd.2                                                 'enable pullup resistors on the inputs
Set Portd.3
Set Portd.4
Set Portd.5

' Input channels: PORT D: 2,3,4,5 mask off other PINS
Iport = Pind And &B00111100

' Enable Monitoring of temperature sensors and I2C devices
Time1_count = 0 : Timer1 = 1sec : Time1_ok = 0
Enable Timer1
Start Timer1

Printprompt
Do
  Gbinp = Inkey()                                           ' get user input
  If Gbinp <> 0 Then                                        ' something typed in?
    Getinput Gbinp                                          ' give input to interpreter
  End If

  Portvalue = Pind And &B00111100

  If Report.repinput = 1 Then
      Reset Report.repinput
      ' We construct an OLD value that has every bit changed to force a report.
      Iport = Portvalue Xor &B00111100
  End If

  If Iport <> Portvalue Then
      Print "input " ; Portvalue ; " " ; Iport
      Iport = Portvalue
      Waitms 25                                             ' debounce
  End If

  If Time1_ok = 1 Then                                      'An interrupt event ?
    Stop Timer1

    If Time1_count = 0 Then
       1wire_monitor
       I2c_monitor
    Elseif Time1_count = 1 Then
       Convallt
    End If

    Reset Time1_ok
    Start Timer1
  End If
Loop

' Timer interrupt
Timer1_int:
   Timer1 = 1sec
   Set Time1_ok
   Toggle Time1_count
Return

End

' Wait for +++ before entering command processor
' This allows all the livebox startup output to be ignored.
' Display something to the user to show we are receiving console characters.

Sub Bootup_wait

  Cursor Off
  Cls
  Lcd "Booting..."
  J = 0                                                     ' number of pluses rcvd

  ' Note: Don't use CHR(0) in a string bascom will think its terminated
  ' and refuse to allow array() overlay access too?!
  Wiggle = "{001}|/-"
  ' \ char on the LCD is a Japanese Yen symbol.  Override
  ' You must define your characters before using any LCD command.
  Deflcdchar 1 , 32 , 16 , 8 , 4 , 2 , 1 , 32 , 32

  I = 1                                                     ' Wiggle index
  K = 0                                                     ' Serial character count overflow ok.
  Do
    Gbinp = Waitkey()
    If Gbinp = "+" Then
       Incr J
    Else
      J = 0
      Incr K
      If K.0 = 1 Then                                       ' every 2nd char
        Lcd Chr(wigglec(i))
        Shiftcursor Left
        Incr I
        If I > Wiggle_len Then
          I = 1
        End If
      End If
    End If
  Loop Until J = 3

  Lcd "OK"
End Sub

'*****************************************************************************
'#### Initialisation ####

' Map channels 1-4 to an appropriate internal relay port.
Sub Reset_relays
  ' Setup Port to relay mapping

  Rport(1) = 0 : Rport(2) = 1 : Rport(3) = 2 : Rport(4) = 3
  For I = 1 To Relays
    Reset Portb.rport(i)
  Next I

End Sub


' Gather ROM ID for all 1-wire devices
Sub 1wire_setup
  Cnt1wire = 1wirecount()
  If Cnt1wire = 0 Then
    Return
  End If

  If Cnt1wire > Max1wire Then
    Cnt1wire = Max1wire
  End If

  I = 1
  Dsid(i) = 1wsearchfirst()
  For J = 1 To Cnt1wire
    I = I + 8
    Dsid(i) = 1wsearchnext()
  Next
End Sub

'*****************************************************************************
'#### COMMAND Processor ####

Sub Printprompt
  Gbpcinputpointer = 1
  Gspcinput = ""
#if Allow_interactive
  If Interactive = 1 Then
    Print ">";
  End If
#endif
End Sub


Sub Getinput(pbbyte As Byte)
   If Pbbyte = &H0D Then                                    ' Map CR to LF
        Pbbyte = &H0A
   Elseif Pbbyte = 127 Then                                 ' Map DEL to BS
        Pbbyte = &H08
   End If

   ' stores bytes from user and wait for LF (&H0A)
   Select Case Pbbyte
      Case &H0A
#if Allow_interactive
        If Interactive = 1 Then
          Print Chr(&H0d) ; Chr(&H0a) ;
        End If
#endif
        Docommand                                           ' analyse command and execute
        Printprompt
#if Allow_interactive
      Case &H08                                             ' backspace ?
         If Interactive = 1 Then
           If Gbpcinputpointer > 1 Then
              Print Chr(&H08);
              Decr Gbpcinputpointer
              Gspcinp(gbpcinputpointer) = 0
           End If
         End If
#endif
      Case Else                                             ' store user-input
         If Gbpcinputpointer < Cpcinput_len Then
            Gspcinp(gbpcinputpointer) = Pbbyte
            Incr Gbpcinputpointer
            Gspcinp(gbpcinputpointer) = 0
#if Allow_interactive
            If Interactive = 1 Then
              Print Chr(pbbyte);                            ' echo back to user
            End If
#endif
         End If
   End Select
End Sub

Sub Docommand
  ' Handle the simple case of a CR/LF and no input.
  If Gbpcinputpointer = 1 Then
    Return
  End If

  Select Case Gspcinput
     Case "version" : Print "rev " ; Fwmajor ; "." ; Fwminor
     Case "report" : Report = &HFF
     Case "report input" : Report.repinput = 1
     Case "report 1wire" : Report.rep1wire = 1
     Case "report ppe" : Report.repppe = 1
     Case "reboot" : Reboot
     Case "1wirereset" : 1wire_setup
#if Allow_interactive
     Case "debug" : Toggle Interactive
     Case "help" : Help
     Case "status 1wire" : 1wire_status
     Case "status relay" : Relay_status
     Case "status input" : Port_status
     Case "status" :
           Relay_status
           Port_status
           1wire_status
#endif
     Case Else
           Pos = Instr(gspcinput , " ")                     ' the space location
           ' If we didn't find a space or the command is too big.
           If Pos = 0 Or Pos > 4 Then
              Return
           End If
           Decr Pos                                         ' Ignore the SPACE
           Command = Mid(gspcinput , 1 , Pos)               ' up to I is CMD
           Pos = Pos + 2                                    ' Argument
           ' test first word
           Select Case Command
                Case "on" : Poweron Pos
                Case "off" : Poweroff Pos
                Case "lcd" : Lcddisplay Pos
                Case "i2c" : I2cmgmt Pos
                Case "urf" : Universalrf Pos
            End Select
      End Select
End Sub

' This is the recommended way to reboot your MICRO
' http://support.atmel.no/bin/customer?=&action=viewKbEntry&id=21
Sub Reboot
  Config Watchdog = 16                                      ' 16ms
  Start Watchdog
  Idle
End Sub

Sub Help
  Print "$Revision$  Available Commands:"
  Print "<relay> = 1-4"
  Print "  DEBUG"
  Print "  VERSION"
  Print "  HELP"
  Print "  REPORT [1WIRE|PPE|INPUT]"
  Print "  1WIRERESET"
  Print "  STATUS [1WIRE|RELAY|INPUT]"
  Print "  REBOOT"
  Print "  ON <relay>"
  Print "  OFF <relay>"
  Print "  LCD <message>"
  Print "  I2C R"
  Print "      Maa   - addr"
  Print "      Baavv - addr/value"
  Print "      Paapv - addr/port/value"
  Print "  URF hex"
End Sub

Sub Relay_status
  For I = 1 To Relays
    Print "Relay " ; I ; " PORT B." ; Rport(i) ; " " ; Portb.rport(i)
  Next I
End Sub

Sub Port_status
  J = 2
  For I = 1 To 4
    Print "Input " ; I ; " PORT D." ; J ; " " ; Portd.j
    Incr J
  Next I
End Sub


' Show what we found on the 1-wire bus
Sub 1wire_status
   If Cnt1wire = 0 Then
     Print "No 1-wire devices detected on bootup"
     Return
   End If

   Print "1-Wire devices found"
   I = 1                                                    ' Index to start of scratchpad data
   J = 8                                                    ' End of Sensor data also the CRC byte
   For K = 1 To Cnt1wire
     Print "ID " ; K ; " ROM ";

    If Dsid(j) = Crc8(dsid(i) , 7) Then
     ' Dump ROM ID
      For B = I To J
        Print Hex(dsid(b));
      Next

    ' Try to ID the chip from its ROM
      Print " ";
      Select Case Dsid(i)
         Case Ds18s20_id : Print "DS18S20";
         Case Ds18b20_id : Print "DS18B20";
         Case Ds1822_id : Print "DS1822";
         Case Else Print "Unknown";
      End Select

      Print " [";
      If Dssign.k = 1 Then
        Print "-";
      End If

      B = Dsvalue(k) Mod 10
      L = Dsvalue(k) / 10
      Print L ; "." ; B ; " C]"

    Else
      Print "BAD CRC"
    End If
    I = I + 8                                               ' Skip to next ROM
    J = J + 8                                               ' Skip to next CRC byte
  Next
End Sub

'*****************************************************************************
'##### I2C SUB SYSTEM #####

' I2C PPE monitor
Sub I2c_monitor
  For Ii = 1 To Ppe_count
     Ik = Ppe_addr(ii) + 1                                  ' Read = WRITE Addr + 1
     I2cstart
     I2cwbyte Ik
     I2crbyte Ij , Nack
     I2cstop

     ' Has the PPE value changed ?
     If Ij <> Ppe_val(ii) Or Report.repppe = 1 Then
       Print "i2c-ppe " ; Ppe_addr(ii) ; " " ; Ppe_val(ii) ; " " ; Ij
       Ppe_val(ii) = Ij
       Reset Report.repppe
     End If
  Next
End Sub

Sub I2cmgmt(pos As Byte)
  ' The gspcinp string begins with the COMMAND type
  D = Gspcinp(pos)
  If D = "R" Then
    Ppe_count = 0
    Return
  End If

  ' The next TWO characters after that are the PPE address.
  Incr Pos                                                  ' I2C Command type
  Commando(1) = Gspcinp(pos)
  Incr Pos
  Commando(2) = Gspcinp(pos)
  Incr Pos
  Commando(3) = 0
  K = Hexval(command)                                       ' PPE Address

  'Print "i2c cmd: " ; D
  'Print "i2c addr: " ; K

  ' PCF8574  - 0 1 0 0 A2 A1 A0 0 - 0x40 to 0x4E
  ' PCF8574A - 0 0 1 1 1 A2 A1 A0 - 0x38 to 0x3F
  ' PCF8574N - 0 0 1 0 0 A2 A1 A0 - 0x20 to 0x27

  ' Add a new ADDRESS to the list of known PPE chips
  If D = "M" Then
      If Ppe_count < Max_ppe Then
        Ppe_count = Ppe_count + 1
        Ppe_addr(ppe_count) = K
      End If
      Return
  End If

  ' Operating upon an existing PPE chip
  ' Find its OFFSET
  ' If an INVALID address is supplied we use the LAST PPE chip registered.
  For Bp = 1 To Ppe_count
     If K = Ppe_addr(bp) Then
        Exit For
     End If
  Next Bp
 'Print "i2c " ; Bp ; " prev " ; Ppe_val(bp)

  Select Case D
  ' PIN MODE
    Case "P":                                               ' P XX Y Z - Address, Pin 0-7, State 0|1
       B = Gspcinp(pos)
       Incr Pos                                             ' Our PIN
       B = B - "0"                                          ' ASCII to Numeric
       'Print "i2c pin:" ; B
       If B > 7 Then
         Return
       End If
       'Print "i2c state:" ; Chr(gspcinp(pos))

       If Gspcinp(pos) = "1" Then                           ' Our STATE
         Set Ppe_val(bp).b
       Else
         Reset Ppe_val(bp).b
       End If
   ' BYTE MODE
    Case "B":
       Commando(1) = Gspcinp(pos)
       Incr Pos
       Commando(2) = Gspcinp(pos)
       Ppe_val(bp) = Hexval(command)                        ' BYTE to write
    Case Else:
       Return
  End Select

  'Print "i2c send:" ; K ; " " ; Ppe_val(bp)
  Stop Timer1
  I2cstart
  I2cwbyte K
  I2cwbyte Ppe_val(bp)
  I2cstop
  Start Timer1
End Sub


'##### INTERNAL RELAY SUB SYSTEM #####

' Channel ID 1,2,3,4 map to internal relays
Sub Poweron(pos As Byte)
  J = Val(gspcinp(pos))
  If J >= 1 And J <= 4 Then
      Set Portb.rport(j)
#if Allow_interactive
  Else
     If Interactive = 1 Then
        Print "Channel " ; J ; " is not valid"
     End If
#endif
   End If
End Sub

Sub Poweroff(pos As Byte)
  J = Val(gspcinp(pos))
  If J >= 1 And J <= 4 Then
      Reset Portb.rport(j)
#if Allow_interactive
  Else
      If Interactive = 1 Then
        Print "Channel " ; J ; " is not valid"
      End If
#endif
   End If
End Sub


'*****************************************************************************
'##### LCB SUB SYSTEM #####

Sub Lcddisplay(pos As Byte)
  Cls                                                       'clear the LCD display
  Lcd Mid(gspcinput , Pos , 8)
  Lowerline                                                 'select the lower line
  Pos = Pos + 8
  Lcd Mid(gspcinput , Pos , 8)
End Sub

'*****************************************************************************
'##### RF SUB SYSTEM #####

'1 byte (Byte 8bit)
Sub Readbyte
  For K = 1 To 2
    ' End of buffer reached and we still need more?
    If Gppos > Gbpcinputpointer Then
      Rferr = 1
#if Allow_interactive
      If Interactive = 1 Then
         Print "ERR: not enough HEX characters"
      End If
#endif
      Return
    End If
    Hexbyte(k) = Gspcinp(gppos)
    Incr Gppos
  Next K
  Hbyte = Hexval(hexstring)
End Sub

'2 bytes (Word 16bit)
Sub Readword
  Call Readbyte
  If Rferr = 1 Then
    Return
  End If
  Hword = Hbyte
  Shift Hword , Left , 8
  Call Readbyte
  Hword = Hword Or Hbyte
End Sub

' 4 bytes (Long 32bit)
Sub Readlong
  Call Readword
  If Rferr = 1 Then
    Return
  End If
  Hlong = Hword
  Shift Hlong , Left , 16
  Call Readword
  Hlong = Hlong Or Hword
End Sub


Sub Setupurf
  Call Readbyte
  If Rferr = 1 Then
    Return
  End If

  Bitsperframe = Hbyte
  If Bitsperframe > Maxencbits Then
#if Allow_interactive
    If Interactive = 1 Then
      Print "Bad encoding of " ; Bitsperframe
    End If
#endif
    Rferr = 1
    Return
  End If

  ' Number of pulse encodings based on bits per frames.
  Enc = 1
  Shift Enc , Left , Bitsperframe

  ' Frames per bitstream byte
  Framesperbyte = 8 / Bitsperframe

  ' Calculate mask and shifts for processing the bitstream byte
  ' Pre calculate to reduce timing issues during transmission
  Bitmask(1) = Enc - 1
  Bitshift(1) = 0
  For I = 2 To Framesperbyte
      Bitmask(i) = Bitmask(i - 1)
      Shift Bitmask(i) , Left , Bitsperframe
      Bitshift(i) = Bitshift(i - 1) + Bitsperframe
  Next I

  ' Populate pulse timing lookup tables
  For I = 1 To Enc
    Call Readword
    If Rferr = 1 Then
      Return
    End If
    Pulsehi(i) = Hword

    Call Readword
    If Rferr = 1 Then
      Return
    End If
    Pulselo(i) = Hword
  Next I

  Call Readbyte
  If Rferr = 1 Then
      Return
  End If
  Burststosend = Hbyte


  Call Readbyte
  If Rferr = 1 Then
      Return
  End If
  Interburstrepeat = Hbyte

  Call Readword
  If Rferr = 1 Then
      Return
  End If
  Interburstdelay = Hword

  Call Readbyte
  If Rferr = 1 Then
      Return
  End If
  Frames = Hbyte

  ' Read in stream bytes
  Streambytes = Frames / Framesperbyte
  I = Frames Mod Framesperbyte
  If I <> 0 Then
     Incr Streambytes
  End If

  ' check for overflow
  If Streambytes > Maxbitstream Then
#if Allow_interactive
      If Interactive = 1 Then
         Print "ERR: stream too large"
         Rferr = 1
      End If
#endif
      Return
  End If

  For I = 1 To Streambytes
    Call Readbyte
    If Rferr = 1 Then
      Return
    End If
    Bitstream(i) = Hbyte
  Next I

  ' Display what we setup
#if Allow_interactive
  If Interactive = 1 Then
    Print "Bits per Frame: " ; Bitsperframe
    Print "Burts to send: " ; Burststosend
    Print "Frames per byte: " ; Framesperbyte
    Print "Pulse Encodings: " ; Enc
    Print "Interburst repeat " ; Interburstrepeat ; " delay: " ; Interburstdelay ; "uS"
    Print "Frames: " ; Frames
    Print "Streambytes: " ; Streambytes

    For I = 1 To Framesperbyte
      Print "mask/shift " ; I ; ": " ; Hex(bitmask(i)) ; " " ; Bitshift(i)
    Next I

    For I = 1 To Enc
      Print "Pulse " ; I ; ": HI " ; Pulsehi(i) ; " LO " ; Pulselo(i)
    Next I
  End If
#endif
End Sub


Sub Xmitrf
     I = Frames
     K = 1
     While I > 0
         J = Framesperbyte
         While J > 0 And I > 0
            B = Bitstream(k) And Bitmask(j)
            Shift B , Right , Bitshift(j)
            Incr B                                          ' Adjust for 1 based indexing

            Set Awire
            Iw = Pulsehi(b)
            Waitus Iw

            Reset Awire
            Iw = Pulselo(b)
            Waitus Iw

            Decr I
            Decr J
         Wend
         Incr K
     Wend
End Sub

Sub Universalrfv1(pos As Byte)
  Setupurf

  If Rferr = 1 Then
#if Allow_interactive
    If Interactive = 1 Then
      Print "RF setup failed"
    End If
#endif
    Return
  End If

  Stop Timer1
  Disable Interrupts
  While Burststosend > 0
    Xmitrf
    For I = 1 To Interburstrepeat
      Waitus Interburstdelay
    Next I
    Decr Burststosend
  Wend
  Enable Interrupts
  Start Timer1

  If Interactive = 1 Then
    Print "RF sent"
  End If
End Sub


Sub Universalrf(pos As Byte)
  Rferr = 0
  Gppos = Pos

  ' Version byte - for future use
  Call Readbyte
  If Rferr = 1 Then
      Return
  End If

  Universalrfv1 Pos
End Sub

'*****************************************************************************
'##### 1WIRE SUB SYSTEM #####

'****************** 1-wire temperature monitoring *********************
Sub 1wire_monitor
  Ij = 1
  For Ii = 1 To Cnt1wire
                                                    ' Only process TEMP sensors
    If Dsid(ij) = Ds18s20_id Or Dsid(ij) = Ds18b20_id Or Dsid(ij) = Ds1822_id Then
      1wverify Dsid(ij)                                     'Issues the "Match ROM "
      If Err = 0 Then
        1wwrite Ds18x20_read
        Sc(1) = 1wread(9)
        If Sc(9) = Crc8(sc(1) , 8) Then
          Call Disp_temp(ii , Ij)
        End If
      End If
    End If

    Ij = Ij + 8
  Next
  Reset Report.rep1wire                                     ' Clear the force report bit.
End Sub


'Makes the Dallas "Convert T" command on the 1w-bus configured in "Config 1wire = Portb. "
'WAIT 200-750 ms after issued, internal conversion time for the sensor
'SKIPS ROM - so it makes the conversion on ALL sensors on the bus simultaniously
'When leaving this sub, NO sensor is selected, but ALL sensors has the actual
'temperature in their scratchpad ( within 750 ms )

Sub Convallt
  If Cnt1wire = 0 Then
    Return
  End If
 1wreset                                                    ' reset the bus
 1wwrite &HCC                                               ' skip rom
 1wwrite Ds18x20_convert_t
End Sub


' Display if the value has changed since last polled.
Sub Disp_temp(cnt As Byte , Offset As Byte)
  Imeas = 0
  Imeas = Makeint(sc(1) , Sc(2))

  ' 18S20 is only 9bit upscale to 12bit
  If Dsid(offset) = Ds18s20_id Then
    Imeas = Imeas And &HFFFE
    Shift Imeas , Left , 3
    Bi = 16 - Sc(6)
    Bi = Bi - 4
    Imeas = Imeas + Bi
  End If

  If Imeas.15 = 1 Then                                      ' if meas & &H8000 then
    Set Dssign.cnt
    ' convert to +ve, (two's complement)++
    Imeas = Imeas Xor &HFFFF
    Incr Imeas
  Else
    Reset Dssign.cnt
  End If


  If Dsid(offset) = Ds18b20_id Then
    Bi = Sc(ds18b20_conf_reg)
    ' clear undefined bit for != 12bit
    If Bi.5 = 1 And Bi.6 = 1 Then                           ' 12 bit
     ' nothing
    Elseif Bi.6 = 1 Then                                    ' 11 bit
      Imeas = Imeas And &HFFFE
    Elseif Bi.5 = 1 Then                                    '10 bit
       Imeas = Imeas And &HFFFC
    Else                                                    ' 9 bit
      Imeas = Imeas And &HFFF8
    End If
  End If

  Temp = Imeas And &HF
  Shift Imeas , Right , 4

  Temp = Temp * Ds18x20_fracconv
  Temp = Temp / 1000
  Imeas = Imeas * 10
  Imeas = Imeas + Temp

  If Dssign.cnt = 1 Then
     Restore Rounding
     For Bi = 1 To 8
        Read Di
        If Temp = Di Then
          Incr Imeas
          Exit For
        End If
     Next
  End If

  ' If this value different from the stored value ?
  ' Or we are forcing a report
  If Dsvalue(cnt) <> Imeas Or Report.rep1wire = 1 Then
      Print "1wire " ;
      ' Dump ROM ID
      Bi = Offset + 8
      For I = Offset To Bi
        Print Hex(dsid(i));
      Next
    Print " ";
    If Dssign.cnt = 1 And Imeas <> 0 Then
      Print "-";
    End If

    Dsvalue(cnt) = Imeas

    Bi = Imeas Mod 10
    Imeas = Imeas / 10

    Print Imeas ; "." ; Bi

  End If

End Sub

Rounding:
Data 1 , 3 , 4 , 6 , 9 , 11 , 12 , 14