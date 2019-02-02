[Return to main README](README.md "Return to main README")


How to autostart this program ?
===============================
when systemd is used :
(files are found in /etc/systemd/system being links to
 the real files in /lib/systemd/system/ )

First you alter mmdvmhost.service by adding the 'Requires' line
This will tell the service it needs nextiondriver.service
before starting MMDVMHost.

mmdvmhost.service
```
[Unit]
Description=MMDVM Host Service
After=syslog.target network.target
Requires=nextiondriver.service

[Service]
User=root
WorkingDirectory=/opt/MMDVMHost
ExecStartPre=/bin/sleep 3
ExecStart=/usr/bin/screen -S MMDVMHost -D -m /opt/MMDVMHost/MMDVMHost /opt/MMDVM.ini
ExecStop=/usr/bin/screen -S MMDVMHost -X quit

[Install]
WantedBy=multi-user.target
```



Then you make a service 'nextiondriver.service'
where you tell it needs to start before MMDVMHost :


nextiondriver.service
```
[Unit]
Description=NextionDriver service
DefaultDependencies=no
After=local-fs.target wifi-country.service
Before=timers.target

[Service]
User=root
WorkingDirectory=/opt/MMDVMHost
Type=forking
ExecStart=/opt/NextionDriver/NextionDriver -i -c /opt/MMDVM.ini -vvvv
ExecStop=/usr/bin/killall NextionDriver

[Install]
WantedBy=multi-user.target
WantedBy=network-online.target
RequiredBy=mmdvmhost.service
```

