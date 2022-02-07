/*
 *   Copyright (C) 2017,2022 by Lieven De Samblanx ON7LDS
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

#define NextionDriver_VERSION "1.25"

#define TRUE	1
#define FALSE	0

#define DEBUG	1

#define NEXTIONPORT			""
#define NEXTIONDRIVERLINK		"/dev/ttyNextionDriver"
#define CONFIGFILE			"/etc/MMDVM.ini"
#define CONFIGFILE_NEXTIONDRIVER	"/etc/NextionDriver.ini"
#define GROUPSFILE			"groups.txt"
#define GROUPSFILESRC			"https://api.brandmeister.network/v1.0/groups/"
#define USERSFILE			"users.csv"
#define USERSFILESRC			"https://www.radioid.net/static/user.csv"
#define MAXGROUPS	2500
#define MAXUSERS	250000

#define MODEM_DISPLAY 0
#define MMDVM_DISPLAY 1

#define BAUDRATE3	B9600
#define BAUDRATE4	B115200

#define SERBUFSIZE 	1024

typedef struct groupdata
{
    int   nr;
    char *name;
} group_t;

typedef struct userdata
{
    int   nr;
    char *data1;
    char *data2;
    char *data3;
    char *data4;
    char *data5;
} user_t;

typedef struct user_idx_data{
	char *call;
	int nr;
} user_call_idx_t;

extern char TXbuffer[1024],RXbuffer[1024];

extern long sleepTimeOut;
extern int page,statusval,changepages,removeDim,sleepWhenInactive,showModesStatus,waitForLan,sendUserDataMask;
extern int check;
extern char ipaddr[100];
extern int modeIsEnabled[20];
extern int netIsActive[10];
extern char groupsFile[100],usersFile[100];
extern char groupsFileSrc[200],usersFileSrc[200];
extern group_t groups[MAXGROUPS];
extern user_t users[MAXUSERS];
extern int nmbr_groups, nmbr_users;

extern unsigned int RXfrequency,TXfrequency;
extern char location[90];
extern unsigned char tempInF;

extern user_call_idx_t usersCALL_IDX[MAXUSERS];

extern char remotePort[10],localPort[10];
extern int transparentIsEnabled,sendFrameType;
extern char datafiledir[500];

extern char nextionDriverLink[100];
extern int verbose, screenLayout;
extern char OSname[100],PIname[100];

extern char nextionPort[100];
extern char configFile1[200],configFile2[200];

extern char userDBDelimiter;
extern int userDBId,userDBCall,userDBName,userDBX1,userDBX2,userDBX3;

extern int display_TXsock;
extern int display_RXsock;
extern struct addrinfo* display_addr;

char* RGBtoNextionColor(int RGB);
void sendCommand(char *cmd);
void writelog(int level, char *fmt, ...);




#endif