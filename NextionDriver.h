/*
 *   Copyright (C) 2017,2020 by Lieven De Samblanx ON7LDS
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

#define NextionDriver_VERSION "1.18"

/* the code to extract RX and TX freq and location has been
    included in MMDVMHost 20180910 and later, so
    it is not necessary to do it with NextionDriver any more.

    If you do want NextionDriver to handle these parameters,
    uncomment the following line                                  */
//#define XTRA


#define TRUE	1
#define FALSE	0

#define DEBUG	1

#define NEXTIONPORT		"/dev/ttyAMA0"
#define NEXTIONDRIVERLINK	"/dev/ttyNextionDriver"
#define CONFIGFILE		"/etc/MMDVM.ini"
#define GROUPSFILE		"groups.txt"
#define USERSFILE		"stripped.csv"
#define MAXGROUPS	1500
#define MAXUSERS	200000

#define MODEM_DISPLAY 0
#define MMDVM_DISPLAY 1

#define BAUDRATE3	B9600
#define BAUDRATE4	B115200


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
extern int page,statusval,changepages,removeDim,sleepWhenInactive,showModesStatus,waitForLan;
extern int check;
extern char ipaddr[100];
extern int modeIsEnabled[20];
extern int netIsActive[10];
extern char groupsFile[100],usersFile[100];
extern group_t groups[MAXGROUPS];
extern user_t users[MAXUSERS];
extern int nmbr_groups, nmbr_users;


#ifdef XTRA
extern unsigned int RXfrequency,TXfrequency;
extern char location[90];
#endif


extern user_call_idx_t usersCALL_IDX[MAXUSERS];

extern char remotePort[10],localPort[10];
extern int transparentIsEnabled,sendFrameType;
extern char datafiledir[500];

extern char nextionDriverLink[100];
extern int verbose, screenLayout;

extern char nextionPort[100];
extern char configFile[200];

extern char userDBDelimiter;
extern int userDBId,userDBCall,userDBName,userDBX1,userDBX2,userDBX3;

extern int display_TXsock;
extern int display_RXsock;
extern struct addrinfo* display_addr;

char* RGBtoNextionColor(int RGB);
void sendCommand(char *cmd);
void writelog(int level, char *fmt, ...);




#endif