/*
 *   Copyright (C) 2017...2021 by Lieven De Samblanx ON7LDS
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

#if !defined(helpers_H)
#define helpers_H

#define C_DSTAR		1
#define C_DMR		2
#define C_YSF		3
#define C_YSFDMR	4
#define C_P25		5
#define C_NXDN		6
#define C_POCSAG	7
#define C_DSTARNET	11
#define C_DMRNET	12
#define C_YSFNET	13
#define C_YSFDMRNET	14
#define C_P25NET	15
#define C_NXDNNET	16

#define C_INFO		20
#define C_NEXTION	21
#define C_LOG		22
#define C_TRANSPARENT	23
#define C_NEXTIONDRIVER	24

void getNetworkInterface(char* info);
int getInternetStatus(int);
pid_t proc_find(const char* name);
void initConfig(void);
int readConfig(int);
void addLH(char*);
void sendScreenData(unsigned int);
void LHlist(int,int);
void dumpLHlist(void);
void readGroups(void);
void readUserDB(void);
void updateDB(int age);
void readUsersGroups(void);
void print_groups(void);
void print_users(void);
void print_call_id(void);
void readVersions(char *filename);
int getDiskFree(int log);
int getHWinfo(char* info);
int getMeminfo(char* info);
int openTalkingSocket(void);
int openListeningSocket(void);
int sendTransparentData(int display, char* msg);

int search_group(int nr, group_t a[], int m, int n);
int search_user_index_for_ID(int id, user_t a[], int m, int n);
int search_user_index_for_CALL(char* call, user_call_idx_t a[], int m, int n);


#endif
