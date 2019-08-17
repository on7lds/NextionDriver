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


- [How to use the NextionDriver](README-using.md "How to use the NextionDriver")
- [Modem connected displays](README-modemdisplays.md "Modem connected displays")
- [Autostart NextionDriver](README-starting.md "Autostart NextionDriver")
- [Configuration file options](README-options.md "Configuration file options")
- [Sendig commands from display to host](README-commands.md "Sendig commands from display to host")
- [Change the program by coding yourself](README-coding.md "Change the program by coding yourself")

