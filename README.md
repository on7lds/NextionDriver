NextionDriver (for MMDVMHost)
=============================

The purpose of this program is to provide additional control for
Nextion display layouts other than the MMDVMHost supplied layouts.
It does this by sitting between MMDVMHost and the Nextion Display.
This program takes the commands, sent by MMDVMHost and translates,
changes, adds or removes these commands.

Since the program has to read MMDVM.ini to know the Layout (for
setting the baudrate) and other parameters (i.e. for knowing
the frequency and location), the parameters for the NextionDriver
itself are also located in the MMDVM configuration file,
in an extra section [NextionDriver].

As stated above, the NextionDriver program will change the commands
as needed and adds extra info (i.e. temperature, TG's info, ...) 
and sends this to the Nextion display.

This program also checks the network interface regularly, and it will
show the most recent IP address, so you can check if the IP address
changed.

When the files 'groups.txt' and 'stripped.csv' are present, usernames
and talkgroup names will be looked up and sent to the display. 
NOTE1 : both files have to be sorted in ascending ID order ! 
NOTE2 : for the user data lookup to work, you MUST switch off 
         the DMRID lookup of MMDVMHost (check README-examples
         in the Nextion subdirectory)

The program also has the ability of receiving commands from the Nextion
display. This way, one can provide buttons on a layout and do something
in the host when such a button is pressed.
One could, for example, make a 'system' page on the Nextion with system
info and buttons to restart MMDVMHost, reboot or poweroff the host, ...

Yes, it is possible (when NextionDriver is running) to start/stop/restart
MMDVMHost with buttons on the Nextion display !

When the Nextion display is connected to the serial port of the modem,
NextionDriver V1.03 and later is able to address that port. 
How does dis work ?
Normally, the data to the display goes directly from MMDVMHost to the
modem, tagged as data for the serial port. The modem will then pass the
data to the serial port.
By enabling transparent data, we are able to inject data into MMDVMHost
and we will tag it as data for the display (this can only be done by
enabling sendFrameType).

Data from the Nextion (when pressing buttons) will be treated similar and
will exit MMDVMHost as transparent data.
***IMPORTANT*** : the firmware of the modem has to pass data from the display
to MMDVMHost for this to work!


How to use this program ?
=========================

| Pi-Star users : check my website on7lds.net |
| ------------------------------------------- |


after compiling with  
```sh
   $ make
```
you should have a binary  
```
   NextionDriver
```
  
You can start this program in _debug mode_, then all commands that are sent
to the display, will be printed to stdout.
Or you can start this program in _normal mode_, then it will go to the 
background and do its work quietly (start and stop of the program
will be logged in syslog)  

The most practical way to start is by specifying only one parameter:
 the place of the MMDVMHost configuration file. 
This way, all configuration can be done in the ini file.

```sh
$ ./NextionDriver -c /etc/MMDVM.ini
```

will start NextionDriver from the current directory and read all parameters from
the MMDVMHost ini file.


You can get the necessary changes in your MMDVMHost configuration file by
executing the patch script

```sh
$ ./NextionDriver_ConvertConfig <MMDVMHost configuration file>
```

Then the script will make a backup of your current config and try to do the changes for you.


In case you want to do it by hand :
In your MMDVMHost configuration file (mostly MMDVM.ini), make a section as below:

```
[NextionDriver]
Port=/dev/ttyAMA0
LogLevel=2
DataFilesPath=/opt/NextionDriver/
GroupsFile=groups.txt
DMRidFile=stripped.csv
RemoveDim=0
SleepWhenInactive=600
ShowModesStatus=0
```


**IMPORTANT**
In the MMDVM.ini [Nextion] section you have to specify the NextionDriver's
virtual port as the port to connect to :
```
[Nextion]
Port=/dev/ttyNextionDriver
...
```
to tell MMDVMHost to talk to our program an not directly to the display !


Modem-connected displays
========================
As of V1.03 the NextionDriver has support for Nextion displays which are
connected to the modem ('Port=modem' in MMDVM.ini)
For this, it is *necessary* to use the MMDVMHost code dated 20180815 or later
(G4KLX GitID #f0ea25d or later). 
It surely is OK when the MMDVMHost version string is 20180910 or later.

In the MMDVM.ini file, you **must** enable 'Transparent Data'
 **and** it's option 'SendFrameType', i.e. :

```
[Transparent Data]
Enable=1
RemoteAddress=127.0.0.1
RemotePort=40094
LocalPort=40095
SendFrameType=1
```

Then you can instruct NextionDriver to use the Transparent Data
to connect to the display. This is done by setting the 'Port' option in
the NextionDriver section of MMDVM.ini to 'modem'
```
[NextionDriver]
Port=modem
...
```


NextionDriver Options
=====================
|Option           |Explanation                                         |
|-----------------|----------------------------------------------------|
|Port             |the port to which the Nextion display is connected  |
|LogLevel         |0 (no logging) ... 4 (verbose logging)              |
|DataFilesPath    |where the users and group info files reside         |
|GroupsFile       |name of the file with talkgroup number <-> name info|
|DMRidFile        |name of the file with user number <-> name info     |
|RemoveDim        |do not pass 'dim' commands to the display           |
|SleepWhenInactive|sleep when no data received for ... seconds         |
|                 |  (display goes to black), 0 = never sleep          |
|ShowModesStatus  |send colors to show status of modes like pi-star    |


How to autostart this program ?
===============================
when systemd is used :
(files are found in /etc/systemd/system being links to
 the real files in /lib/systemd/system/ )

First you alter mmdvmhost.service by adding the 'BindsTo' line
This will tell the service it needs nextion-helper.service
before starting MMDVMHost

mmdvmhost.service
```
[Unit]
Description=MMDVM Host Service
After=syslog.target network.target
BindsTo=nextion-helper.service

[Service]
User=root
WorkingDirectory=/opt/MMDVMHost
ExecStartPre=/bin/sleep 3
ExecStart=/usr/bin/screen -S MMDVMHost -D -m /opt/MMDVMHost/MMDVMHost /opt/MMDVM.ini
ExecStop=/usr/bin/screen -S MMDVMHost -X quit

[Install]
WantedBy=multi-user.target
```



Then you make a service 'nextion-helper.service'
where you tell it needs to start before MMDVMHost :


nextion-helper.service
```
[Unit]
Description=Nextion Helper Service Service
After=syslog.target network.target
Before= mmdvmhost.service

[Service]
User=root
WorkingDirectory=/opt/MMDVMHost
Type=forking
ExecStart=/opt/NextionDriver/NextionDriver -c /opt/MMDVM.ini
ExecStop=/usr/bin/killall NextionDriver

[Install]
WantedBy=multi-user.target
```

How to change this program to your needs ?
==========================================

The program has a lot of functions included, but those who want to add even 
more could do so.
There are 2 files you could change :

Data to the Nextion Display
---------------------------
the routine process() in processCommands.c

* this routine is called for each command, sent by MMDVMHost. The command
  which is sent, is in RXbuffer (without the trailing 0xFF 0xFF 0xFF).
* the RXbuffer holds a string, so it is not possible to send 0x00 characters
* you can inspect, change or add commands, but keep in mind that at the end of
  the routine the RXbuffer (if not empty) is sent to the Nextion display
  (empty the buffer if you do not want to send something to the display)

Data from the Nextion Display
-----------------------------
the routine processButtons() in processButtons.c

* This routine is called whenever there is an event sent from the display.
  For this you have to make a button on the Nextion display which has in its
  Touch Release Event following code:  
   printh 2A  
   printh (code nr)  
   printh FF  
   printh FF  
   printh FF  

where (code nr) is a number 0x01...0xEF (look sharp: 0xEF NOT 0xFF !)
The command is in the RXbuffer (without the trailing 0xFF 0xFF 0xFF).

Then there are some special codes :

* when there is a command  
   printh 2A  
   printh F0  
   printh (linux command)  
   printh FF  
   printh FF  
   printh FF  
  
the 'linux command' is executed  
  
* When there is a command  
   printh 2A  
   printh F1  
   printh (linux command)  
   printh FF  
   printh FF  
   printh FF  
  
the 'linux command' is executed and the __FIRST__ line of the result
is sent to the display variable 'msg'  

There is an example HMI file included.   
Press on the MMDVM logo on the main screen to go to the 'system' page  
  
  
* When there is a command  
   printh 2A  
   printh FE  
   printh (number)  
   printh FF  
   printh FF  
   printh FF  
  
page (number) will be refreshed, that is : all __text__ fields for this page 
from the last time the page was shown, will be (re)sent to the display.  
Note: this will only be for fields which have a name in the range t0 ... t39
 
* When there is a command  
    printh 2A  
    printh FD  
    printh (page)  
    printh (number)  
    printh FF  
    printh FF  
    printh FF  
  
the (number) last heard fields of (page) are sent.  

Note : __EXPERIMENTAL !__ (works with some flaws)

For now : page __has to be 2__ and number <20  
The fiels will be sent and the fieldnames are prepended with 'LH(nr)'  
So: LH0t0 LH0t1 LH0t2 ... LH1t0 LH1t1 LH1t2 ... LH2t0 LH2t1 LH2t2 ...

* When there is a command  
   printh 2A  
   printh FC  
   printh (number)  
   printh FF  
   printh FF  
   printh FF  
  
the inhibit mode wil be set (number=1) / reset (number=0)  
When inhibit is set to active (1), the current RX buffer will be processed 
and then all data from MMDVMHost will be dropped.
This can be practical when changing the Nextion page to your settings page. 
Then no page changes or field data from MMDVMHost will be sent to the display.
When inhibit is reset (0) all data from then on will be processed.

* When there is a command  
   printh 2A  
   printh FB  
   printh FF  
   printh FF  
   printh FF  
  
NextionDriver will search for a .tft file in the DataFilesPath directory 
where the filename matches the connected Nextion display model.
i.e. : when the a display NX4832K035_011R is connected, NextionDriver will 
search for a file NX4832K035.tft and try to load that into the display.
At this moment, baud rate switching is not yet supported. So the .tft file 
will be loaded at the active display baudrate (9600 for layout<4 and 115200 for 
layout 4)
