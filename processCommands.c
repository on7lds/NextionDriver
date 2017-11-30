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
#include "NextionDriver.h"

#include <time.h>

int page;

//============================================================================
//if you want to use Nextion colors but are used to RGB:
// this function tranforms RGB to nextion 5/6/5 bit
char* RGBtoNextionColor(int RGB){
    static char color[10];
    unsigned int R=(RGB>>16)&0x0000FF;
    unsigned int G=(RGB>> 8)&0x0000FF;
    unsigned int B=(RGB>> 0)&0x0000FF;
    int n=(((R>>3)<<11) + ((G>>2)<<5) + ((B>>3)));
    sprintf(color,"%d",n);
    return color;
}


//send command (in TXbuffer) to Nextion
//and if answer != 0 wait for a Nextion answer (in RXbuffer, length is returned)
int sendCommand(char *cmd,int answer){
    int received=0;

    if (strlen(cmd)>0) {
        write(fd2,cmd,strlen(cmd));
        write(fd2,"\xFF\xFF\xFF",3);

        if (!become_daemon) printf("TX: %s\n",cmd);
        fflush(NULL);
    }
    if (answer>0) {
        usleep(100000);	//Nextion needs some time to send answer
        received=read(fd2,RXbuffer,100);
        if ((RXbuffer[received-1]==255)&&(RXbuffer[received-2]==255)&&(RXbuffer[received-3]==255)) 
            { received-=3; RXbuffer[received]=0; }
    }
    return received;
}


void showRXbuffer(int buflen) {
    int z;
    if (!become_daemon) printf("RX: ");
    for (z=0; z<buflen;z++) {
        if ((RXbuffer[z]<' ')||(RXbuffer[z]>254)) {
            if (!become_daemon) printf("0x%02X ",RXbuffer[z]); 
        } else {
            if (!become_daemon) printf("%c ",RXbuffer[z]);
        }
    }
    if (!become_daemon)  {printf("\n");fflush(NULL);}
}
//============================================================================





//============================================================================
//
//   Put your code in the subroutine below
//
//============================================================================
void process() {
    char tmp[100];
    int r;

    //process commands
    //----------------

/*
    //simple example :
    //change color of TA
    if (strcmp(TXbuffer,"t2.pco=1024")==0) {
        sprintf(TXbuffer,"t2.pco=%s",RGBtoNextionColor(0x0000FF));
        printf("Tx: %s\n",TXbuffer);
    }
    if (strcmp(TXbuffer,"t2.pco=33808")==0) {
        sprintf(TXbuffer,"t2.pco=%s",RGBtoNextionColor(0x00FF00));
        printf("Tx: %s\n",TXbuffer);
    }
*/



    //more complex example :
    //This is a proof of concept to mimick part of the EA5SW display
    // changes when connected to a 'normal' MMDVMHost

    if (strcmp(TXbuffer,"page MMDVM")==0) {
        page=0;
        sendCommand("t30.txt=\"Valencia - IM99TK\"",0);
        sendCommand("t31.txt=\"439.9625\"",0);
	sprintf(tmp,"MMDVM.bco=%s",RGBtoNextionColor(0x0000FF));
        sendCommand(tmp,0);
//        sendCommand("t36.pco=60965",0);
        sendCommand("t36.txt=\"Idle\"",0);
    }
    if ((page==0)&&(strstr(TXbuffer,"t3.txt=")>0)) {
        strcpy(tmp,"t33.txt=");
        strcat(tmp,strstr(TXbuffer,"t3.txt="));
    }
    if ((page==0)&&(strstr(TXbuffer,"t2.txt=")>0)) {
        FILE *temperatureFile;
        double T;
        temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
        if (temperatureFile == NULL)
          ; //print some message
        fscanf (temperatureFile, "%lf", &T);
        T /= 1000;
        char text1[30U];
        sprintf(text1, "t32.txt=\"Temp:%6.3f\"", T);
        sendCommand(text1,0);
        fclose (temperatureFile);
    }

    if (strcmp(TXbuffer,"page DMR")==0) {
        page=1;

    }

    if ((page==1)&&(strcmp(TXbuffer,"t2.font=4")==0)) {
        strcpy(TXbuffer,"t2.font=11");
    }
    if ((page==1)&&(strcmp(TXbuffer,"t2.txt=\"2 Listening\"")==0)) {
        strcpy(TXbuffer,"t2.txt=\"DMR RX\"");
    }



    // send command and check for data from Nextion
    r=sendCommand(TXbuffer,1);

    // // // // // // // // // // // // // // // // // // // // // // //
    // execute command from buttonpress on Nextion
    // // // // // // // // // // // // // // // // // // // // // // //

    // for case 3
    time_t timer;
    struct tm* tm_info;


    if (r>1) {
        if (!become_daemon) printf("RX: %d\n",r);
        if (RXbuffer[0]==42) {
            if (!become_daemon) { printf("Received command %d\n",RXbuffer[1]); fflush(NULL); }
            switch (RXbuffer[1]) {
                case 0:
                    sendCommand("t0.txt=\"Will stop MMDVMHost\"",0);
                    system("service mmdvmhost stop");
                    break;
                case 1:
                    sendCommand("t0.txt=\"Will start MMDVMHost\"",0);
                    system("service mmdvmhost start");
                    break;
                case 2:
                    time(&timer);
                    tm_info = localtime(&timer);
                    strftime(TXbuffer, 100, "t0.txt=\"%d-%m-%Y %H:%M:%S\"", tm_info);
                    sendCommand(TXbuffer,0); 
                    break;
                case 3:
                    sendCommand("t0.txt=\"Will reboot\"",0);
                    system("reboot");
                    break;
                case 4:
                    sendCommand("t0.txt=\"Will poweroff\"",0);
                    system("poweroff");
                    break;
            }
        }
    }
}

