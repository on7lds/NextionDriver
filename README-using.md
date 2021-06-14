[Return to main README](README.md "Return to main README")


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
DMRidFile=users.csv
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
to tell MMDVMHost to talk to our program and not directly to the display !

