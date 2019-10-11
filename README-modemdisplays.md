[Return to main README](README.md "Return to main README")


Modem-connected displays
========================
As of V1.03 the NextionDriver has support for Nextion displays which are
connected to the modem ('Port=modem' in MMDVM.ini)
For this, it is *necessary* to use the MMDVMHost code dated 20180815 or later
(G4KLX GitID #f0ea25d or later). 
It surely is OK when the MMDVMHost version string is 20180910 or later.

NOTE:  When connecting a Nextion to the Nextion port on a MMDVM_HS hat,
the Nextion screen default baud rate must be set to 9600 unless you
build custom firmware and define a higher UART2 speed.  Send the
following serial command in the Nextion simulator "bauds=9600"

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

Data from the Nextion (when pressing buttons) will be treated similar and 
will exit MMDVMHost as transparent data. 
__IMPORTANT__ : the firmware of the modem has to pass data from the display
 to MMDVMHost for this to work!

If this is not (yet) the case, you could ask the developer of your modem to 
add this feature.
