[Return to main README](README.md "Return to main README")


Sending commands from the display
=================================

#### Execute a linux command:
```
    2A F0 (linux command) FF FF FF
```
The 'linux command' is executed.

#### Execute a linux command: with feedback:
```
   2A F1 (linux command) FF FF FF
or
   2A F1 (number) (linux command) FF FF FF
```
The 'linux command' is executed and the __FIRST__ line of the result
is sent to the display variable 'msg'

There is an example HMI file included.
Press on the MMDVM logo on the main screen to go to the 'system' page
  
After executing the linux command, status.val=25 is sent and S0.click 
is triggered. This allows the display to process the command result, if 
necessary.
  
If the third byte, (number) is present (with number 1..31), the result
of the command will not be returned in msg.txt, but in the variable 
resultXX.txt where XX = the number that was given.
  
Example : 2A F1 05 date FF FF FF wil return the date/time in result05.txt
  
#### Refresh page
```
   2A FE (number) FF FF FF
```
Page (number) will be refreshed, that is : all __text__ fields for this page 
from the last time the page was shown, will be (re)sent to the display.  
Note: this will only be for fields which have a name in the range t0 ... t39
  
#### Last Heard
```
   2A FD (page) (number) FF FF FF
```
The (number) last heard fields of (page) are sent.  

Note : __EXPERIMENTAL !__ (works with some flaws)

For now : page __has to be 2__ and number <20  
The fiels will be sent and the fieldnames are prepended with 'LH(nr)'  
So: LH0t0 LH0t1 LH0t2 ... LH1t0 LH1t1 LH1t2 ... LH2t0 LH2t1 LH2t2 ...
  
#### Display Control
```
   2A FC (number) FF FF FF
```
The inhibit mode wil be set (number=1) / reset (number=0)  
When inhibit is set to active (1), the current RX buffer will be processed 
and then all data from MMDVMHost will be dropped.
This can be practical when changing the Nextion page to your settings page. 
Then no page changes or field data from MMDVMHost will be sent to the display.
When inhibit is reset (0) all data from then on will be processed.

When number=2, the display model and serial number are sent in 'msg1' and 
'msg2' respectively.

The S0 click event will be activated after executing the command, with
status value 25.


#### Info & Updates
```
   2A FB (number) FF FF FF
```
  
##### (number) = 0 : update display
NextionDriver will search for a .tft file in the DataFilesPath directory 
where the filename matches the connected Nextion display model.
i.e. : when the a display NX4832K035_011R is connected, NextionDriver will 
search for a file NX4832K035.tft and try to load that into the display.
At this moment, baud rate switching is not yet supported. So the .tft file 
will be loaded at the active display baudrate (9600 for layout<4 and 115200 for 
layout 4)  
  
##### (number) = 1 : update users and groups files
NextionDriver will check the age of the users and group files. Then NextionDriver 
will try to update the files which are older than a day.  
The groups file will be fetched from BrandMeister.  
The users file will be fetched from RadioID.  
  
##### (number) = 2: get age info about users and groups files
NextionDriver will check the age of the users and group files.  
Results are sent to 'msg1' (groups file) and 'msg2' (users file)
  
##### (number) = 3 : update OS
Not yet implemented
  
##### (number) = 4: get info about OS
NextionDriver will check the OS version and Pi-Star version.  
If it is not running on Pi-Star, it will say so.  
Results are sent to 'msg1' (os version) and 'msg2' (Pi-Star version)
  
##### (number) = 5 : get info about hardware
NextionDriver will check the hardware it is runnuing on.  
The boardinfo (if available) or CPU info will be sent to 'msg1'.  
The memory size in kB will be sent to 'msg2'.  
  
