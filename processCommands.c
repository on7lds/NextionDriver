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


//simple example :
//change color of TA when using display layout 2 in MMDVM.ini
/*
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
// changes when connected to a 'normal' MMDVMHost program
/*
    char tmp[100];
    int page;

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

*/

}

