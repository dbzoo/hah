'********************************************************************
'* Serial Support
'* Language:       BASCOM-AVR 1.11.9.1
'*
'* $Id$
'********************************************************************


$regfile = "m8def.dat"

' Allow interactive debug mode - increases code size.
Const Allow_interactive = 0

' Code thats useful but takes up too much flash for a MEGA-8
' allow_interactive must be 1 before considering extended support.
Const Extended_interactive = 0

' production or AVR-SDK1 dev
Const Production = 1

' Compiling for the custom SERIAL PCB
#if Production

$crystal = 7372800                                          ' External XTAL
$baud = 115200

#else

' Compiling for the AVR-SDK board
$crystal = 4000000
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

Declare Sub Avr_os()
Declare Sub Reset_relays()
Declare Sub Printprompt()
Declare Sub Docommand()
Declare Sub Getinput(byval Pbbyte As Byte)
Declare Sub Intpoweron(chnl As Byte)
Declare Sub Intpoweroff(chnl As Byte)
Declare Sub Rfpoweron(byval Chnl As Byte)
Declare Sub Rfpoweroff(byval Chnl As Byte)
Declare Sub Poweron(byval Chnl As Byte)
Declare Sub Poweroff(byval Chnl As Byte)
Declare Sub Lcddisplay(pos As Byte)
Declare Sub Sendcode(byval Chnl As Byte , Byval Onoff As Byte)
#if Extended_interactive
Declare Sub Help()
Declare Sub Relay_status()
Declare Sub 1wire_status()
Declare Sub Dump_eeprom()
#endif

Declare Sub 1wire_setup()
Declare Sub Docode(byval Idx As Byte)
Declare Sub Storerf(pos As Byte)
Declare Sub Bootup_wait()
Declare Sub Reboot
Declare Sub Convallt                                        ' Convert T on ALL sensors
Declare Sub Meas_to_cel(id As Byte , Offset As Byte)
Declare Sub 1wire_monitor
Declare Sub Disp_temp(cnt As Byte , Offset As Byte)
Declare Sub I2cmgmt(pos As Byte)
Declare Sub I2c_monitor

' I want a timer that resolves to 1sec, so the first step is to divide down
' the input clock .  Input clock to divide by 256, so now each clock pulse is
' worth (7.3728/256)=28.8uS.

' The trick is to preload the counter with a value that will cause it to count
' for 750mS. Each time the counter expires, you can force the new number back
' into the timer register. We need 1/.0000288 or 34722 counts. However, the
' counter expires at 65536, so the correct number to preload is 65536-34722 or
' 30813 This way, the timer will count up 34722 counts (1s) and expire.
#if Production
Const 1sec = 30813                                          ' @ 7.3728Mhz
#else
Const 1sec = 65472                                          ' @ 4Mhz
#endif

' Temperator sensor configuration
Const Ds18b20_conf_reg = 4

' constant to convert the fraction bits to cel*(10^-4)
Const Ds18x20_fracconv = 625

' DS18x20 ROM ID
Const Ds18s20_id = &H10
Const Ds18b20_id = &H28

' COMMANDS
Const Ds18x20_convert_t = &H44
Const Ds18x20_read = &HBE
Const Ds18x20_write = &H4E
Const Ds18x20_ee_write = &H48
Const Ds18x20_ee_recall = &HB8
Const Ds18x20_read_power_supply = &HB4

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

#if Production
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
#else
' PORT D
'   0 : DB4 (LCD)
'   1 : DB5 (LCD)
'   2 : DB6 (LCD)
'   3 : DB7 (LCD)
'   4 : E   (LCD)
'   5 : RS  (LCD)
'   6 : n/c
'   7 : n/c
Config Lcdpin = Pin , Db4 = Portd.4 , Db5 = Portd.5 , Db6 = Portd.6 , Db7 = Portd.7 , E = Portd.3 , Rs = Portd.2
#endif
Config Lcd = 16 * 1a                                        'configure lcd screen

#if Production
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
#else
' PORT C
'   0 : Input 1 - Button 1
'   1 : Input 2 - Button 2
'   2 : Input 2 - Button 3
'   3 : Input 2 - Button 4
'   4 :
'   5 :
'   6 :
'   7 :

Config Pinc.0 = Input , Pinc.1 = Input , Pinc.2 = Input , Pinc.3 = Input
#endif

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
Const Cpcinput_len = 30                                     ' max. length of user-Input
Dim Gspcinput As String * Cpcinput_len                      ' holds user-input
Dim Gspcinp(cpcinput_len) As Byte At Gspcinput Overlay
Dim Gbpcinputpointer As Byte                                ' string-pointer during user-input

' 1-wire devices
Const Max1wire = 15
' Up to 15 each having an 8 byte ROMID
Dim Dsid(120) As Byte                                       ' Dallas ID 64bit inc. CRC
Dim Dsvalue(max1wire) As Word                               ' Value of each sensor
Dim Dssign As Word                                          ' If max1wire > 7 make this a word
Dim Cnt1wire As Byte                                        ' Number of 1-wire devices found

' I2C PPE devices
Const Max_ppe = 8
Dim Ppe_count As Byte
Ppe_count = 0
Dim Ppe_addr(8)as Byte
Dim Ppe_val(8) As Byte                                      ' Last read value value

#if Extended_interactive
' Bootup wiggler
Const Wiggle_len = 4
Dim Wiggle As String * Wiggle_len
Dim Wigglec(wiggle_len) As Byte At Wiggle Overlay
#endif

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
Dim Hilo As Byte
Dim Bp As Byte
Dim D As Byte
Dim B As Byte

' Variables used by the INTERRUPT routine - Don't use these for general purpose
' as their values will unexpectedly change.
Dim Ii As Byte
Dim Ij As Byte
Dim Ik As Byte
Dim Bi As Byte
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
Const Maxchannel = 16                                       ' First 4 are reservered for onboard RELAYS
Const Rflen = 144                                           ' = (maxchannel-4) * 12
Dim Rf(rflen) As Byte

' Setup default RF remote control sequences
$eeprom
' This can be trashed by the AVR on reset or startup (DONT USE)
' Also as BASCOM is 1 based indexing this is ADDRESS 0 which make mapping difficult.
Data &H00
' We begin at EEPROM Address 1
Data &H4D , &H2A , &HAA , &HAA , &HAA , &H80                ' channel 5 off
Data &H4D , &H2A , &HAA , &HAB , &H2B , &H00                ' channel 5 on
Data &H4D , &H2A , &HAA , &HCA , &HAC , &H80                ' channel 6 off
Data &H4D , &H2A , &HAA , &HCB , &H2D , &H00                ' channel 6 on
Data &H4D , &H2A , &HAA , &HB2 , &HAB , &H00                ' channel 7 off
Data &H4D , &H2A , &HAA , &HB3 , &H2A , &H80                ' channel 7 on
Data &H4D , &H2A , &HAA , &HD2 , &HAD , &H00                ' channel 8 off
Data &H4D , &H2A , &HAA , &HD3 , &H2C , &H80                ' channel 8 on
$data

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

' Read RF EEPROM control data into RAM for transmission
For I = 1 To Rflen
   Readeeprom Rf(i) , I
Next I

#if Production
Set Portd.2                                                 'enable pullup resistors on the inputs
Set Portd.3
Set Portd.4
Set Portd.5

' Input channels: PORT D: 2,3,4,5 mask off other PINS
Iport = Pind And &B00111100
#else
' Input buttons
Iport = Pinc And &B00001111
#endif

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

#if Production
  Portvalue = Pind And &B00111100
#else
  Portvalue = Pinc And &B00001111
#endif

  If Report.repinput = 1 Then
      Reset Report.repinput
      ' We construct an OLD value that has every bit changed to force a report.
#if Production
      Iport = Portvalue Xor &B00111100
#else
      Iport = Portvalue Xor &B00001111
#endif
  End If

  If Iport <> Portvalue Then
      Print "input " ; Portvalue ; "," ; Iport
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

#if Extended_interactive
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
#else
  Do
    Gbinp = Waitkey()
    If Gbinp = "+" Then
       Incr J
    Else
      J = 0
    End If
  Loop Until J = 3
#endif

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
     Case "report" : Report = &HFF
     Case "reboot" : Reboot
     Case "1wirereset" : 1wire_setup
#if Allow_interactive
     Case "debug" : Toggle Interactive
#if Extended_interactive
     Case "eeprom" : Dump_eeprom
     Case "help" :
        If Interactive = 1 Then
          Help
        End If
     Case "status" :
        If Interactive = 1 Then
           Relay_status
           1wire_status
        End If
#endif
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
                Case "rf" : Storerf Pos
            End Select
      End Select
End Sub

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

  ' for PCF8574A chips we may need to adjust this
  If K < &H40 Or K > &H47 Then                              ' Sanity check
    Return
  End If

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

' This is the recommended way to reboot your MICRO
' http://support.atmel.no/bin/customer?=&action=viewKbEntry&id=21
Sub Reboot
  Config Watchdog = 16                                      ' 16ms
  Start Watchdog
  Idle
End Sub

#if Extended_interactive
Sub Help
  Print "$Revision: 1.15 $  Available Commands:"
  Print "<channel> = 1-8"
  Print "  HELP"
  Print "  REPORT"
  Print "  1WIRERESET"
  Print "  EEPROM"
  Print "  STATUS"
  Print "  REBOOT"
  Print "  ON <cc/channel> "
  Print "  OFF <cc/channel>"
  Print "  LCD <message>"
  Print "  I2C R"
  Print "      Maa   - addr"
  Print "      Baavv - addr/value"
  Print "      Paapv - addr/port/value"
  Print "  RF ccpbbbbbbbbbbbb"
End Sub
#endif

' Channel ID 1,2,3,4 map to internal relays
' Channel ID 5,6,7,8 map to external RF Controlled relays

Sub Poweron(pos As Byte)
  J = Val(gspcinp(pos))
  Select Case J
   Case 1 To 4:
      Call Intpoweron(j)
   Case 5 To Maxchannel:
      Call Rfpoweron(j)
#if Allow_interactive
   Case Else
     If Interactive = 1 Then
        Print "Channel " ; J ; " is not valid"
     End If
#endif
   End Select
End Sub

Sub Poweroff(pos As Byte)
  J = Val(gspcinp(pos))
  Select Case J
   Case 1 To 4:
      Call Intpoweroff(j)
   Case 5 To Maxchannel:
      Call Rfpoweroff(j)
#if Allow_interactive
   Case Else
      If Interactive = 1 Then
        Print "Channel " ; J ; " is not valid"
      End If
#endif
   End Select
End Sub

#if Extended_interactive
Sub Relay_status
  For I = 1 To Relays
    Print "relay " ; I ; " ";
    If Portb.rport(i) = 1 Then
       Print "1"
    Else
       Print "0"
    End If
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
         Case Else Print "Unknown";
      End Select

      Print " [";
      If Dssign.k = 1 Then
        Print "-";
      End If
      Print Dsvalue(k) ; " C]"

    Else
      Print "BAD CRC"
    End If
    I = I + 8                                               ' Skip to next ROM
    J = J + 8                                               ' Skip to next CRC byte
  Next
End Sub
#endif

'*****************************************************************************

'##### INTERNAL RELAY SUB SYSTEM #####

Sub Intpoweron(chnl As Byte)
  Set Portb.rport(chnl)
End Sub

Sub Intpoweroff(chnl As Byte)
  Reset Portb.rport(chnl)
End Sub

'##### LCB SUB SYSTEM #####

Sub Lcddisplay(pos As Byte)
  Cls                                                       'clear the LCD display
  Lcd Mid(gspcinput , Pos , 8)
  Lowerline                                                 'select the lower line
  Pos = Pos + 8
  Lcd Mid(gspcinput , Pos , 8)
End Sub

'##### RF SUB SYSTEM #####


#if Extended_interactive
Sub Dump_eeprom
  If Interactive = 0 Then
    Return
  End If
  D = 1

  ' RF Channel's start at 5 to MAXCHANNEL
  For I = 5 To Maxchannel
    For J = 1 To 2
      Print "0x" ; Hex(i) ; "(" ; I ; ")";                  ' Display RF offset
      If J = 1 Then
        Print "OFF: ";
      Else
        Print "ON : ";
      End If
      For K = 1 To 6
        Readeeprom B , D
        Incr D
        Print " " ; Hex(b) ;
      Next K
      Print
    Next J
  Next I

End Sub
#endif

Sub Storerf(pos As Byte)
'  rf <chnl><on/off><6 bytes in HEX >
'  rf ccpbbbbbbbbbbbb - cc HEX Channel, p On/Off indicator, bbb.. RF control
'
'  Example:
'     The default for Channel 10 (RF05) OFF
'  rf 0504D2AAAAAAA80

  ' Not the correct number of bytes?
  If Gbpcinputpointer < 18 Then
#if Allow_interactive
    If Interactive = 1 Then
      Print "Not enough values"
    End If
#endif
    Return
  End If

  ' Indexing from 0 (RF 0 = Channel 5)
  Commando(1) = Gspcinp(pos)
  Incr Pos
  Commando(2) = Gspcinp(pos)
  Incr Pos
  Commando(3) = 0
  J = Hexval(command)
  J = J - 5                                                 ' Users start a 5 we start a 0.
  J = J * 12
  Hilo = Gspcinp(pos) - "0"
  If Hilo = 1 Then
    J = J + 6
  End If
  Incr Pos
  Incr J                                                    ' Offset into the RF byte array
  'Sanity check
  If J > Rflen Then
    Return
  End If

  ' Expect 6 bytes of EEPROM data for a channel
  L = 0                                                     ' Need to write to ERAM indicator
  For I = 1 To 6
    Commando(1) = Gspcinp(pos)
    Incr Pos
    Commando(2) = Gspcinp(pos)
    Incr Pos
    K = Hexval(command)
    If K <> Rf(j) Then
      Rf(j) = K
      L = 1
    End If
    Incr J
  Next I

  ' Has a change in data been detected?
  If L = 1 Then
#if Allow_interactive
    If Interactive = 1 Then
      Print "WR EEPROM"
    End If
#endif
    K = J - 6
    For I = K To J
      Writeeeprom Rf(i) , I
    Next I
#if Extended_interactive
    Dump_eeprom
#endif
  End If
End Sub

Sub Rfpoweron(byval Chnl As Byte)
     Call Sendcode(chnl , 1)
End Sub

Sub Rfpoweroff(byval Chnl As Byte)
     Call Sendcode(chnl , 0)
End Sub

Sub Sendcode(chnl As Byte , Onoff As Byte)
  Stop Timer1                                               ' Don't interrupt pulse timings

  ' Find index into the RF data.
  ' Each chunk is 6 bytes long
  ' [chnl 1 off][chnl 1 on][chnl 2 off][chnl 2 on]...
  ' 1         6 7        12 13      18 19       24
  '
  ' RF 2 (channel 6) on worked example
  ' b = (6 - 5)      RF units start at Channel 5 normalize.
  ' b = 1 * 12       Offset to channel section
  ' b = 12 + 6       Adjust for On/Off stream
  ' b = 18 + 1       BASCOM uses 1 based indexing
  ' b = 19           Index
  Pos = Chnl - 5
  Pos = Pos * 12
  If Onoff = 1 Then
    Pos = Pos + 6
  End If
  Incr Pos

  ' Send the RF sequence multiple times.
  For I = 1 To 8
    Call Docode(pos)
    Waitms 80                                               ' Interburst delay
  Next I

  Start Timer1
End Sub

' Send an RF bit stream
Sub Docode(byval Idx As Byte)
  Reset Awire
  Bp = 7                                                    'Start a Bit Position 7 MSB
  For J = 1 To 42
    D = Rf(idx).bp + 1                                      ' bit 0|1 is 1|2 units of pulse
    Hilo = Bp Mod 2                                         ' Even bits HI, Odds bit LOW

    ' Send RF pulse of length D
    For K = 1 To D
      If Hilo = 1 Then
         Set Awire
      Else
         Reset Awire
      End If
      Waitus 680
    Next K

    ' Examine next control BIT
    Decr Bp
    ' Have we iterated from bit 7-0 already?
    If Bp = 255 Then
      Incr Idx                                              'Next RF control Sequence
      Bp = 7                                                'Reset to BIT position 7
    End If
  Next
  Reset Awire
End Sub


'****************** 1-wire temperature monitoring *********************

Sub 1wire_monitor
  Ij = 1
  For Ii = 1 To Cnt1wire
    If Dsid(ij) = Ds18s20_id Or Dsid(ij) = Ds18b20_id Then  ' Only process TEMP sensors
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
    Print "1wire " ; Cnt ; " ";
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