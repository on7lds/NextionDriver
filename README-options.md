[Return to main README](README.md "Return to main README")


NextionDriver Options
=====================

These are the options that can be specified in the [NextionDriver] section
of the MMDVMHost ini file.



### Port
specifies the port to which the Nextion display is connected 

This could be (on linux) :
- /dev/AMA0
- /dev/ttyUSB0
- ...

### LogLevel
specifies the level of logging.  
This can be 0 (no logging) ... 4 (verbose logging)


### DataFilesPath
specifies the path where the users and group info files reside


### GroupsFile
name of the file with talkgroup number <-> name info

**Note:**  
The existence of the groups.txt file is also used for the calculation of free space.  
This means: the free space that is sent to the display, is the free space
of the partition where the groups.txt file is.  
Without the groups.txt file, free space is not calculated.


### DMRidFile
the name of the file with user number <-> name info

### DMRidDelimiter
the field delimiter character of the DMRis file  
default:  
DMRidDelimiter=,

### DMRidId, DMRidCall, DMRidName, DMRidX1, DMRidX2, DMRidX3

which data fields to take from the DMRid file.  
default :  
DMRidId=1  
DMRidCall=2  
DMRidName=3  
DMRidX1=4  
DMRidX2=5  
DMRidX3=7  

The extra field (X1, X2, X3) are the extra data you want to display. This an be
city, region, country or city, state, country, or ...

IMPORTANT:
Each line in the DMRid file has to contain 2 .. 9 fields (more will not be read).  
The absolute minimum is, of course, DMRid and call.


### RemoveDim
if set to 1, the 'dim' commands are not passed to the display.
So MMDVMHost does not set the display backlight any more. You can do it yourself
 by coding it in the display.


### SleepWhenInactive
sleep when no serial data received for specified number of seconds (display goes to black)  
0 = never sleep  
The main goal is to signal the death of MMDVMHost and then spare the screen
 (mainly the backlight) when there is nothing to show any more.


### ShowModesStatus
send colors to show status of modes like pi-star


### WaitForLan
show seconds counter while waiting for LAN and show ip address when LAN becomes active 
(even before start of MMDVMHost)

**Note:**  
The NextionDriver will not execute commands from the display while waiting.
They are buffered and executed when the waiting is over.
