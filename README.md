NextionDriver (for MMDVMHost)
=============================

The purpose of this program is to provide additional control for
Nextion display layouts other than the MMDVMHost supplied layouts.
It does this by sitting between MMDVMHostand the Nextion Display.
This program takes the commands, sent by MMDVMHost and translates, changes,
adds or removes these commands.

For example: it should be possible to connect an EA5SW layout to an unmodified
MMDVMHost program.
The NextionDriver program will change the commands as needed and adds extra
info (i.e. temperature, TG's info, ...) and sends this to the Nextion display.

The program also has the ability of receiving commands from the Nextion
display. This way, one can provide buttons on a layout and do something
in the host when such a button is pressed.
One could, for example, make a 'system' page on the Nextion with system info
and buttons to restart MMDVMHost, reboot or poweroff the host, ...


How to change this program to your needs ?
------------------------------------------

For the moment, this is a 'proof of concept', so you'll have to modify a bit
yourself.

You should only have to change the code in processCommands.c , more specific
the routine 'process()'

* this routine is called for each command, sent by MMDVMHost. The command
  which is sent, is in RXbuffer (without the trailing 0xFF 0xFF 0xFF).
* you can inspect, change or add commands, but keep in mind that at the end of
  the routine the RXbuffer (if not empty) is sent to the Nextion display
* at the end of the routine, as an example, the data from the Nextion is
  checked and if it is '0x2A 0x??' then a command is executed, depending of
  the 0x?? byte
  Therefore you make a button on the Nextion display which has in its
  Touch Release Event following code (change the 0x00 byte to 0x01 or 0x02,
  etc for other buttons) :
	printh 2A
	printh 00
	printh FF
	printh FF
	printh FF

  There is an example HMI file, based on one of the EA5SW layouts, with
  a 'system' page which will let you stop or start MMDVMHost and poweroff
  or reboot the host.


How to use this program ?
-------------------------

after compiling with
	make

you should have a binary
	NextionDriver

You can start this program in debug mode, then all commands that are sent
to the display will be printed to stdout.
Or you can start this program in normal mode, then it will go to the 
background and do its work quitly (start and stop of the program
will be logged in syslog)

The program takes at least 2 parameters. For example :

./NextionDriver -n /dev/ttyAMA0 -m /dev/NextionDriver

- will start NextionDriver from the current directory
- the programm will use the serial port /dev/ttyAMA0 for communication
  with the display
- the program will create a /dev/NextionDriver link as a virtual serial
  port where it will listen
  
***IMPORTANT***
In your MMDVM.ini you go to the Nextion section and specify:

[Nextion]
Port=/dev/NextionDriver
...

to tell MMDVMHost to talk to our program !


How to autostart this program ?
-------------------------------
when systemd is used :
(files are found in /etc/systemd/system being links to
 the real files in /lib/systemd/system/ )

First you alter mmdvmhost.service by adding the 'BindsTo' line
This will tell the service it needs nextion-helper.service

:::::::::::::::::::::mmdvmhost.service:::::::::::::::::::::
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
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


Then you make a service 'nextion-helper.service'
where you tell it needs to start before MMDVMHost :


::::::::::::::::::nextion-helper.service:::::::::::::::::::
```
[Unit]
Description=Nextion Helper Service Service
After=syslog.target network.target
Before= mmdvmhost.service

[Service]
User=root
WorkingDirectory=/opt/MMDVMHost
Type=forking
ExecStart=/opt/NextionDriver/NextionDriver -n /dev/ttyAMA0 -m /dev/NextionDriver
ExecStop=/usr/bin/killall NextionDriver

[Install]
WantedBy=multi-user.target
```
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

