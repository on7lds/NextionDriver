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

#if !defined(helpers_H)
#define helpers_H

#define DSTAR		1
#define DMR		2
#define FUSION		3
#define P25		4
#define YSFDMR		5
#define NXDN		6
#define DSTAR_NET	7
#define DMR_NET		8
#define FUSION_NET	9
#define P25_NET		10
#define YSFDMR_NET	11
#define NXDN_NET	12


void getNetworkInterface(char* info);
int getInternetStatus(int);
pid_t proc_find(const char* name);
int readConfig(void);
void readGroups(void);
void readUserDB(void);
void print_users(void);
int getDiskFree(void);

int search_group(int nr, group_t a[], int m, int n);
int search_userID(int nr, user_t a[], int m, int n);
int search_userCALL(char* call, user_t a[], int m, int n);


#endif
