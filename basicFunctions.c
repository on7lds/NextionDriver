/*
 *   Copyright (C) 2017,2018 by Lieven De Samblanx ON7LDS
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>

#include "NextionDriver.h"
#include "helpers.h"


void basicFunctions() {

    char text[100];

    //---------------------------------------------------
    // the 'page' variable holds the last selected page
    //---------------------------------------------------
    if (strcmp(TXbuffer,"page MMDVM")==0) {
        page=0;
    }
    if (strcmp(TXbuffer,"page DStar")==0) {
        page=1;
    }
    if (strcmp(TXbuffer,"page DMR")==0) {
        page=2;
    }
    if (strcmp(TXbuffer,"page YSF")==0) {
        page=3;
    }
    if (strcmp(TXbuffer,"page P25")==0) {
        page=4;
    }


    //--------------------------------------------------------------
    //                  Rest page
    //--------------------------------------------------------------
    //  * remove Freq and time when stopping
    //  * regularly check IP interface and send to display
    //  * get CPU temperature & frequency & load average 
    //       and send to display
    //  * send RX frequency and location (info from MMDVM.ini)
    //--------------------------------------------------------------
    if ((page==0)&&(strstr(TXbuffer,"MMDVM STOPPED")>0)){
        sprintf(TXbuffer, "t23.txt=\"\"");
        sendCommand(text);
        sprintf(TXbuffer, "t2.txt=\"\"");
    }
    // if date/time is sent, check IP interface from time to time:
    if ((page==0)&&(strstr(TXbuffer,"t2.txt=")>0)&&(check++>100)) {
        getNetworkInterface(ipaddr);
        sprintf(TXbuffer, "t3.txt=\"%s\"", ipaddr);
        check=0;
    }
    // check temp & CPU freq (also not too often)
    if ((page==0)&&(strstr(TXbuffer,"t2.txt=")>0)&&(check%8==0)) {
        FILE *deviceInfoFile;
        double val;
        //CPU temperature
        deviceInfoFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
        if (deviceInfoFile == NULL) {
            sprintf(text, "t20.txt=\"?\"");
        } else {
            fscanf (deviceInfoFile, "%lf", &val);
            val /= 1000;
            sprintf(text, "t20.txt=\"%2.2f %cC\"", val, 176);
            fclose(deviceInfoFile);
        }
        sendCommand(text);
        //CPU frequency
        deviceInfoFile = fopen ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq", "r");
        if (deviceInfoFile == NULL) {
            sprintf(text, "t21.txt=\"?\"");
        } else {
            fscanf (deviceInfoFile, "%lf", &val);
            val /= 1000;
            sprintf(text, "t21.txt=\"%0.0f MHz\"", val);
            fclose(deviceInfoFile);
        }
        sendCommand(text);
        //CPU loadavg 1 min
        deviceInfoFile = fopen ("/proc/loadavg", "r");
        if (deviceInfoFile == NULL) {
            sprintf(text, "t22.txt=\"?\"");
            sendCommand(text);
            sprintf(text, "cpuload.val=0");
        } else {
            fscanf (deviceInfoFile, "%lf", &val);
            sprintf(text, "t22.txt=\"%0.2f\"", val);
            sendCommand(text);
            sprintf(text, "cpuload.val=%d", (int)(val*100));
            fclose(deviceInfoFile);
        }
        sendCommand(text);
        //RXFrequency
        float fx;
        fx=frequency;
        fx/=1000000;
        sprintf(text, "t23.txt=\"%3.4fMHz\"",fx);
        sendCommand(text);

        //Location
        sprintf(text, "t24.txt=\"%s\"",location);
        sendCommand(text);

        //Done
        sprintf(text, "MMDVM.status.val=20");
        sendCommand(text);
        sendCommand("click S0,1");

    }

    //send TG name if found
    if ((page==2)&&(strstr(TXbuffer,"t3.txt")!=NULL)) {
        char *TGname;
        int nr,TGindex;
        nr=atoi(&TXbuffer[10]);
        TGindex=search_group(nr,groups,0,nmbr_groups-1);
            sendCommand(TXbuffer);
        if (TGindex>=0) {
            TGname=groups[TGindex].name;
            sprintf(TXbuffer,"t8.txt=\"%s\"",TGname);
        } else sprintf(TXbuffer,"t8.txt=\"TG%d name not found\"",nr);
    }

    //send user data if found
    if ((page==2)&&(strstr(TXbuffer,"t2.txt")!=NULL)) {
        int nr,user;

        sendCommand(TXbuffer);

        nr=atoi(&TXbuffer[12]);
        user=search_user(nr,users,0,nmbr_users-1);
        if (user>=0) {
            sprintf(TXbuffer,"t13.txt=\"%s\"",users[user].data1);
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t14.txt=\"%s\"",users[user].data2);
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t15.txt=\"%s\"",users[user].data3);
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t16.txt=\"%s\"",users[user].data4);
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t17.txt=\"%s\"",users[user].data5);
            sendCommand(TXbuffer);

        } else if (nr>0) {
            sprintf(TXbuffer,"t13.txt=\"DMRID %d\"",nr);
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t14.txt=\"Not found in\"");
            sendCommand(TXbuffer);
            sprintf(TXbuffer,"t15.txt=\"%s\"",USERSFILE);
            sendCommand(TXbuffer);
        }
        sprintf(text, "MMDVM.status.val=78");
        sendCommand(text);
        sendCommand("click S0,1");
    }
}

