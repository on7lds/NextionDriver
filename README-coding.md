[Return to main README](README.md "Return to main README")


Change the program to your needs by coding yourself
===================================================

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

####When you want to write code youself

the routine processButtons() in processButtons.c

This routine is called whenever there is an event sent from the display.
  For this you have to make a button on the Nextion display which has in its
  Touch Release Event following code:..
   printh 2A
   printh (code nr)
   printh FF
   printh FF
   printh FF

where (code nr) is a number 0x01...0xEF (look sharp: 0xEF NOT 0xFF !)
The command is in the RXbuffer (without the trailing 0xFF 0xFF 0xFF).

