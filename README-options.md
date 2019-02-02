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
name of the file with user number <-> name info


### RemoveDim
if set to 1, the 'dim' commands are not passed to the display.
So MMDVMHost does not set the display backlight any more. You can do it yourself
 bij coding it in the display.


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
