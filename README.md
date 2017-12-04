NextionDriver (for MMDVMHost)
=============================

The purpose of this program is to provide additional control for
Nextion display layouts other than the MMDVMHost supplied layouts.
It does this by sitting between MMDVMHost and the Nextion Display.
This program takes the commands, sent by MMDVMHost and translates,
changes, adds or removes these commands.

For example: it should be possible to connect an EA5SW layout to an
unmodified MMDVMHost program.
The NextionDriver program will change the commands as needed and adds 
extra info (i.e. temperature, TG's info, ...) and sends this to 
the Nextion display.

The program also has the ability of receiving commands from the Nextion
display. This way, one can provide buttons on a layout and do something
in the host when such a button is pressed.
One could, for example, make a 'system' page on the Nextion with system
info and buttons to restart MMDVMHost, reboot or poweroff the host, ...


How to change this program to your needs ?
==========================================

There are 2 files you could change :

Data to the Nextion Display
---------------------------
the routine process() in processCommands.c

* this routine is called for each command, sent by MMDVMHost. The command
  which is sent, is in RXbuffer (without the trailing 0xFF 0xFF 0xFF).
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
	printh <code nr>
	printh FF
	printh FF
	printh FF

  where <code nr> is a number 0x01...0xEF (look sharp: 0xEF NOT 0xFF !)
  The command is in the RXbuffer (without the trailing 0xFF 0xFF 0xFF).	

  Then there are some special codes :

* when there is a command
	printh 2A
	printh F0
	printh <linux command>
	printh FF
	printh FF
	printh FF

	the 'linux command' is executed

* When there is a command 
	printh 2A
	printh F1
	printh <linux command>
	printh FF
	printh FF
	printh FF

	the 'linux command' is executed and the __FIRST__ line of the result
	is sent to the display variable 'msg'

  There is an example HMI file included.
  Press on the MMDVM logo on the main screen to go to the 'system' page


How to use this program ?
=========================

after compiling with
	make

you should have a binary
	NextionDriver

You can start this program in _debug mode_, then all commands that are sent
to the display, will be printed to stdout.
Or you can start this program in _normal mode_, then it will go to the 
background and do its work quitly (start and stop of the program
will be logged in syslog)

The program takes at least 2 parameters. For example :

./NextionDriver -n /dev/ttyAMA0 -m /dev/ttyNextionDriver

- will start NextionDriver from the current directory
- the programm will use the serial port /dev/ttyAMA0 for communication
  with the display
- the program will create a /dev/ttyNextionDriver link as a virtual serial
  port where it will listen

**IMPORTANT**
In your MMDVM.ini you go to the Nextion section and specify:
```
[Nextion]
Port=/dev/ttyNextionDriver
...
```
to tell MMDVMHost to talk to our program !


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
ExecStart=/opt/NextionDriver/NextionDriver -n /dev/ttyAMA0 -m /dev/ttyNextionDriver
ExecStop=/usr/bin/killall NextionDriver

[Install]
WantedBy=multi-user.target
```
