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
#include <sys/vfs.h>

#include "NextionDriver.h"



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
				writelog(LOG_NOTICE," IPv4: %s", interfacelist[ifnr]);
			} else {
				sprintf(interfacelist[ifnr], "%s: %s", ifa->ifa_name, host);
				writelog(LOG_NOTICE," IPv6: %s", interfacelist[ifnr]);
			}

			ifnr++;
		}
	}

	freeifaddrs(ifaddr);

	writelog(LOG_NOTICE," Default interface is : %s" , dflt);

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

int readConfig(void) {
    #define BUFFER_SIZE 200
    int ok=0;

    FILE* fp = fopen(configFile, "rt");
    if (fp == NULL) {
        fprintf(stderr, "Couldn't open the MMDVM.ini file - %s\n", configFile);
        return 0;
    }
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
    // since this is for Nextion displays, we assume Nextion is enabeled,
    //  so we do not check this and only search for the screenLayout variable
    if (buffer[0U] == '#')
        continue;
        if (buffer[0U] == '[') {
            ok=0;
            if (strncmp(buffer, "[Info]", 6U) == 0) ok=1;
            if (strncmp(buffer, "[Nextion]", 9U) == 0) ok=2;
            if (strncmp(buffer, "[Log]", 5U) == 0) ok=3;
        }
        char* key   = strtok(buffer, " \t=\r\n");
        if (key == NULL)
            continue;

        char* value = strtok(NULL, "\r\n");
        if (value == NULL)
            continue;

        if (ok==1) {
            if (strcmp(key, "RXFrequency") == 0)
                frequency = (unsigned int)atoi(value);
            if (strcmp(key, "Location") == 0)
                strcpy(location,value);
        }
        if (ok==2) {
            if (strcmp(key, "ScreenLayout") == 0)
                screenLayout = (unsigned int)atoi(value);
        }
/*
        if (ok==3) {
            if (strcmp(key, "FileLevel") == 0)
                loglevel = (unsigned int)atoi(value);
        }
*/
    }
    fclose(fp);

    return 1;
}


int getDiskFree(void){
  struct statfs sStats;
  char fname[250];

  strcpy(fname,datafiledir);
  strcat(fname,GROUPSFILE);

  if( statfs( fname, &sStats ) == -1 )
    return -1;
  else
  {
    int size,free;
    //sizes in MB;
    size=((sStats.f_blocks/1024)*sStats.f_bsize)/1024;
    free=((sStats.f_bavail/1024)*sStats.f_bsize)/1024;
    // in PCT
    return (100*free)/size;
  }
}




int search_group(int nr, group_t a[], int m, int n)
{
    writelog(LOG_DEBUG,"--- Group search for %d (%d - %d)",nr,m,n);
    if (nr==0) return -1;
    if (m>n) return -1;

    if(m == n) {
        if(a[n].nr == nr) { return n; } else  return -1;
    }

    if((n-m)<2) {
        if(a[n].nr == nr) { return n; } else  return -1;
    }

    int middle=(m+n)/2;
    if(a[middle].nr==nr)  return middle;
    else
    if(nr > a[middle].nr) return search_group(nr, a, middle, n);
    else
    if(nr < a[middle].nr) return search_group(nr, a, m, middle);

    return -1;
}


int search_user(int nr, user_t a[], int m, int n)
{
    writelog(LOG_DEBUG,"--- User search for %d (%d [%d] - %d [%d] )",nr,m,a[m].nr,n,a[n].nr);
    //usleep(200000);
    if (nr==0) return -1;
    if (m>n) return -1;

    if((n-m)<2) {
        if(a[n].nr == nr) { return n; } else  return -1;
    }

    int middle=(m+n)/2;
    if(a[middle].nr==nr)  return middle;
    else
    if(nr > a[middle].nr) return search_user(nr, a, middle, n);
    else
    if(nr < a[middle].nr) return search_user(nr, a, m, middle);

    return -1;
}


void print_groups() {
    int i;

    for (i=0; i<nmbr_groups;i++)
        printf("Group %5d: %d = [%s]\n", i, groups[i].nr, groups[i].name);
}

void print_users() {
    int i;

    for (i=0; i<nmbr_users;i++)
        printf("User %5d: %d = [%s][%s][%s][%s][%s]\n", i, users[i].nr, users[i].data1, users[i].data2, users[i].data3, users[i].data4, users[i].data5);
}


void insert_user(user_t table[], int nr, void *new_data1, void *new_data2, void *new_data3, void *new_data4, void *new_data5)
{
    char *m;

    m = malloc(1024);
    if (m==NULL) return;
    free(m);

    table[nmbr_users].nr = nr;

    int size;
    size=strlen(new_data1)+1;
    table[nmbr_users].data1 = malloc(size);
    memcpy(table[nmbr_users].data1,new_data1,size);

    size=strlen(new_data2)+1;
    table[nmbr_users].data2 = malloc(size);
    memcpy(table[nmbr_users].data2,new_data2,size);

    size=strlen(new_data3)+1;
    table[nmbr_users].data3 = malloc(size);
    memcpy(table[nmbr_users].data3,new_data3,size);

    size=strlen(new_data4)+1;
    table[nmbr_users].data4 = malloc(size);
    memcpy(table[nmbr_users].data4,new_data4,size);

    size=strlen(new_data5)+1;
    table[nmbr_users].data5 = malloc(size);
    memcpy(table[nmbr_users].data5,new_data5,size);
    nmbr_users++;
}



void insert_group(group_t table[], int nr, void *new_data){

    table[nmbr_groups].nr = nr;

    int size;
    size=strlen(new_data)+1;
    table[nmbr_groups].name = malloc(size);
    memcpy(table[nmbr_groups].name,new_data,size);
    nmbr_groups++;
}

void readGroups(void){
#define BUFFER_SZ 100
    char fname[250];

    strcpy(fname,datafiledir);
    strcat(fname,GROUPSFILE);

    writelog(LOG_NOTICE,"Reading groups from %s",fname);

    nmbr_groups=0;

    FILE* fp = fopen(fname, "rt");
    if (fp == NULL) {
        writelog(LOG_ERR, "Couldn't open the groups file %s.",fname);
        return;
    }

    char buffer[BUFFER_SZ];
    int nr;

    while (fgets(buffer, BUFFER_SZ, fp) != NULL) {
        nr=0;
        char* key   = strtok(buffer, " \":");
        if (key == NULL) continue;
        nr=atoi(key);

        key   = strtok(NULL, ":\"");
        if (key == NULL)
            continue;

        key   = strtok(NULL, ":\"");
        if (key == NULL) continue;

//        printf("Pushing %d %s \n",nr, key);fflush(NULL);
        insert_group(groups, nr, key);
    }
    fclose(fp);
    writelog(LOG_NOTICE,"Read %d groups.",nmbr_groups);
}


void readUserDB(void){
#define BUFFER_SZ 100
    char fname[250];

    strcpy(fname,datafiledir);
    strcat(fname,USERSFILE);
    writelog(LOG_INFO,"Reading users from %s",fname);

    nmbr_users=0;

    FILE* fp = fopen(fname, "rt");
    if (fp == NULL) {
        writelog(LOG_ERR, "Couldn't open the userDB file %s.",fname);
        return;
    }

    char buffer[BUFFER_SZ];
    int nr,i;
    char *key[8],*next;
    char *niks = "";

    while (fgets(buffer, BUFFER_SZ, fp) != NULL) {

        buffer[strlen(buffer)-1]=0;

        nr=0;

        next = strtok(buffer, ",");
        if (next == NULL) continue;
        nr=atoi(next);

        for(i=0;i<8;i++)key[i]=niks;
        i=0;
        while((i<7)&&(next != NULL))
        {
            key[i++]=next;
            next = strtok(NULL,",");
        }
        if (strlen(key[1])>1) {
//            printf("%5d Pushing [%d] [%s] [%s] [%s] [%s]  [%s]\n",nmbr_users, nr, key[1], key[2], key[3], key[4], key[6]);fflush(NULL);
//            usleep(100000);
            insert_user(users, nr, key[1], key[2], key[3], key[4], key[6]);
        }
    }
    fclose(fp);
    writelog(LOG_INFO,"Read %d users.",nmbr_users);
}
