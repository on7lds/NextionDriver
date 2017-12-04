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

#if !defined(NextionDriver_H)
#define NextionDriver_H

#define NextionDriver_VERSION "0.90"

#define TRUE	1
#define FALSE	0

#define DEBUG	1

#define NEXTIONPORT		"/dev/ttyAMA0"
#define NEXTIONDRIVERLINK	"/dev/ttyNextionDriver"

//#define BAUDRATE	B115200
#define BAUDRATE	B9600

char mux[100];
char mmdvmPort[100];
char nextionPort[100];
char NextionDriverLink[100];


int fd1,fd2;
int become_daemon;

char TXbuffer[1024],RXbuffer[1024];

char* RGBtoNextionColor(int RGB);
void sendCommand(char *cmd);

#endif