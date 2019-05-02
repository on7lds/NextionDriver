#!/bin/bash -x
make clean
make
sudo chgrp bin NextionDriver
sudo chown root NextionDriver
sudo systemctl stop nextiondriver.service
sudo systemctl stop mmdvmhost.service 
sudo cp NextionDriver /usr/local/bin/NextionDriver
sudo systemctl start mmdvmhost.service 
