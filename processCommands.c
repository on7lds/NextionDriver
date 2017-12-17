/*
 *   Copyright (C) 2017 by Lieven De Samblanx ON7LDS
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




void getNetworkInterface(char* info);

//============================================================================
//
//   Put your code in the subroutine below
//
//============================================================================
void processCommands() {

//-------------------------------
//       process commands
//-------------------------------


// ---------------------------------------------------------------------
//   You can study these examples to get an idea of how to program
//   the extra functionality
// ---------------------------------------------------------------------

// When using colors, you can use the function 
//  RGBtoNextionColor( RGBcolor)

// ---------------------------------------------------------------------

    char text[100];

    //the 'page' variable holds the last selected page
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


    // ---------------- Rest page ----------------
    // if date/time is sent, check IP interface from time to time:
    if ((page==0)&&(strstr(TXbuffer,"t2.txt=")>0)&&(check++>100)) {
        getNetworkInterface(ipaddr);
        sprintf(TXbuffer, "t3.txt=\"%s\"", ipaddr);
        check=0;
    }
    // check temp & CPU freq (also not to often)
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
            sendCommand(text);
        } else {
            fscanf (deviceInfoFile, "%lf", &val);
            sprintf(text, "t22.txt=\"%0.2f\"", val);
            sendCommand(text);
            sprintf(text, "cpuload.val=%d", (int)(val*100));
            sendCommand(text);
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
        sprintf(text, "MMDVM.cmd.val=20");
        sendCommand(text);
        sprintf(text, "MMDVM.status.val=18");
        sendCommand(text);
        sendCommand("click S0,1");

    }

    // Change 'Listening' in 'DMR RX if you would want to
/*
    if ((page==1)&&(strcmp(TXbuffer,"t2.txt=\"2 Listening\"")==0)) {
        strcpy(TXbuffer,"t2.txt=\"  DMR RX\"");
    }
*/
}


void getNetworkInterface(char* info) {
	const unsigned int IFLISTSIZ = 25U;
	int n;

	FILE* fp = fopen("/proc/net/route" , "r");
	if (fp == NULL) {
		writelog(LOG_ERR,"Unabled to open /proc/route");
		return;
	}

	char* dflt = NULL;

	char line[100U];
	while (fgets(line, 100U, fp)) {
		char* p1 = strtok(line , " \t");
		char* p2 = strtok(NULL , " \t");

		if (p1 != NULL && p2 != NULL) {
			if (strcmp(p2, "00000000") == 0) {
				dflt = p1;
				break;
			}
		}
	}

	fclose(fp);

	if (dflt == NULL) {
		writelog(LOG_ERR,"Unable to find the default route");
		return;
	}

	char interfacelist[IFLISTSIZ][50+INET6_ADDRSTRLEN];
	for (n = 0U; n < IFLISTSIZ; n++)
		interfacelist[n][0] = 0;

	struct ifaddrs* ifaddr;
	if (getifaddrs(&ifaddr) == -1) {
		writelog(LOG_ERR,"getifaddrs failure");
		return;
	}

	unsigned int ifnr = 0U;
	struct ifaddrs* ifa;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		int family = ifa->ifa_addr->sa_family;
		if (family == AF_INET || family == AF_INET6) {
			char host[NI_MAXHOST];
			int s = getnameinfo(ifa->ifa_addr, family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				writelog(LOG_ERR,"getnameinfo() failed: %s\n", gai_strerror(s));
				continue;
			}

			if (family == AF_INET) {
				sprintf(interfacelist[ifnr], "%s: %s", ifa->ifa_name, host);
				writelog(LOG_INFO," IPv4: %s", interfacelist[ifnr]);
			} else {
				sprintf(interfacelist[ifnr], "%s: %s", ifa->ifa_name, host);
				writelog(LOG_INFO," IPv6: %s", interfacelist[ifnr]);
			}

			ifnr++;
		}
	}

	freeifaddrs(ifaddr);

	writelog(LOG_INFO," Default interface is : %s" , dflt);

	for (n = 0U; n < ifnr; n++) {
		char* p = strchr(interfacelist[n], '%');
		if (p != NULL)
			*p = 0;

		if (strstr(interfacelist[n], dflt) != 0) {
			strcpy((char*)info, interfacelist[n]);
			break;
		}
	}

}

