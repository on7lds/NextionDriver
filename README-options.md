[Return to main README](README.md "Return to main README")


NextionDriver Options
=====================

These are the options that can be specified in the [NextionDriver] section
of the MMDVMHost ini file.



### Port
specifies the port to which the Nextion display is connected 

This could be (on linux) :
- /dev/ttyAMA0
- /dev/ttyUSB0
- modem
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
Without the groups.txt file, free space is calculated for the partition where 
the configuration file resides.


### GroupsFileSrc
This gives the link where the groups file has to be fetched. It must be the complete
link + filename, f.e. :
```
GroupsFileSrc=https://place.of.the.groupsfile/directory/where/groupfile.txt
```
This file will be fetched with `wget` and placed in the directory specified
by `DataFilesPath` and named as specified by `GroupsFile`  
When not specified, the groups file is fetched from BrandMeister.  
**Note:**
The `wget` program must be present on your system !

### DMRidFile
the name of the file with user number <-> name info

### DMRidFileSrc
This gives the link where the DMRid file has to be fetched. It must be the complete
link + filename, f.e. :
```
DMRidFileSrc=https://place.of.the.usersfile/directory/where/DMRid.csv
```
This file will be fetched with `wget` and placed in the directory specified
by `DataFilesPath` and named as specified by `DMRidFile`  
When not specified, the groups file is fetched from RadioID.  
**Note:**
The `wget` program must be present on your system !



### DMRidDelimiter
the field delimiter character of the DMRis file  
default:  
```
DMRidDelimiter=,
```

### DMRidId, DMRidCall, DMRidName, DMRidX1, DMRidX2, DMRidX3

which data fields to take from the DMRid file.  
default :  
```
DMRidId=1  
DMRidCall=2  
DMRidName=3,4  
DMRidX1=5  
DMRidX2=6  
DMRidX3=7  
```


For the DMRidName field, it is possible to specify 2 fields that will be combined
as a name :  
```
DMRidName=3,4  
```
This will use field 3 and add field 4 as name for the user.

The extra field (X1, X2, X3) are the extra data you want to display. This can be
city, region, country or city, state, country, or ...

IMPORTANT:
Each line in the DMRid file has to contain 2 .. 9 fields (more will not be read).  
The absolute minimum is, of course, DMRid and call.


### SendUserDataMask
Choose for which pages all user data (if found) is sent.  
Set the bit for the corresponding page :
0b00000001 DStar  
0b00000010 DMR  
0b00000100 YSF  
0b00001000 P25  
0b00010000 NXDN  
0b00100000 POCSAG  
0b01000000 M17  
  
Numbers may be specified as binary (0b...) or hexadecimal (0x...)  
Default is DMR only : 0b00000010 (binary) or 0x2 (hex).  
  
For example : DMR + YFS :  
```
SendUserDataMask=0b00000110
```
or (this is the same)  
```
SendUserDataMask=0x6
```


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
