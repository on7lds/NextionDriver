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

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/vfs.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#include "NextionDriver.h"
#include "helpers.h"


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
    for (n = 0U; n < IFLISTSIZ; n++) {
        interfacelist[n][0] = 0;
    }

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
                sprintf(interfacelist[ifnr], "%s:%s", ifa->ifa_name, host);
                writelog(LOG_INFO," IPv4: %s", interfacelist[ifnr]);
            } else {
                sprintf(interfacelist[ifnr], "%s:%s", ifa->ifa_name, host);
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


int getInternetStatus(int i){
    char *hostname;
    struct hostent *hostinfo;

    hostname = "brandmeister.network";
    if (i%4 == 1) hostname = "news.brandmeister.network";
    if (i%4 == 2) hostname = "ask.brandmeister.network";
    if (i%4 == 3) hostname = "wiki.brandmeister.network";

    hostinfo = gethostbyname (hostname);

    if (hostinfo == NULL) return 0; else return 1;
}


void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}



void sanitize(char *line) {
    char *new;
    int index,indexN;

    new=strdup(line);
    if (new==NULL) printf("ERROR");

    index=0;
    indexN=0;

    while (line[index]) {
        if ((line[index]!='\'')&&(line[index]!='"')&&(line[index]!=0x0D)&&(line[index]!=0x0A)
            &&(!((line[index]==' ')&&(line[index+1]=='=')))&&(!((index>1)&&(line[index]==' ')&&(line[index-1]=='='))) ) {
            new[indexN++]=line[index];
        }
        index++;
    }
    new[indexN]=0;

    strcpy(line,new);
    free(new);
}



void initConfig(void) {
    int i;

    strcpy(nextionDriverLink,NEXTIONDRIVERLINK);
    strcpy(nextionPort,NEXTIONPORT);
    strcpy(configFile1,CONFIGFILE);
    strcpy(configFile2,"");
    strcpy(groupsFile,GROUPSFILE);
    strcpy(usersFile,USERSFILE);
    strcpy(groupsFileSrc,GROUPSFILESRC);
    strcpy(usersFileSrc,USERSFILESRC);

    screenLayout=2;

    userDBDelimiter=',';
    userDBId=0;
    userDBCall=1;
    userDBName=34;  // fields 2 and 3
    userDBX1=4;
    userDBX2=5;
    userDBX3=6;

    RXfrequency=0;
    TXfrequency=0;
    location[0]=0;
    tempInF=0;

    for(i=0; i<14; i++) modeIsEnabled[i]=0;
    remotePort[0]=0;
    localPort[0]=0;
    transparentIsEnabled=0;
    sendFrameType=0;
    datafiledir[0]=0;
    groupsFile[0]=0;
    usersFile[0]=0;
    changepages=0;

    for (i=0; i<7; i++) modeIsEnabled[i]=0;
}



int readConfig(int filenr) {
    #define BUFFER_SIZE 200
    int found,ok;
    char configFile[200];

    ok=0;
    found=0;

    strcpy(configFile,"");
    if (filenr==1) strcpy(configFile,configFile1);
    if (filenr==2) strcpy(configFile,configFile2);

    if (strlen(configFile)==0) return 1;
    writelog(LOG_NOTICE,"Reading configuration file %s",configFile);

    FILE* fp = fopen(configFile, "rt");
    if (fp == NULL) {
        return -100-filenr;
    }

    changepages=0;
    removeDim=0;
    sleepWhenInactive=60;
    waitForLan=1;
    sendUserDataMask=0b00000010;

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        sanitize(buffer);
        // since this is for Nextion displays, we assume Nextion is enabled,
        //  so we do not check this and only search for the screenLayout variable
        if (buffer[0] == '#') {
            continue;
        }
        if (buffer[0] == '[') {
            ok=0;
            if (strncmp(buffer, "[D-Star]", 8) == 0)             ok=C_DSTAR;
            if (strncmp(buffer, "[DMR]", 5) == 0)                ok=C_DMR;
            if (strncmp(buffer, "[System Fusion]", 15) == 0)     ok=C_YSF;
            if (strncmp(buffer, "[P25]", 5) == 0)                ok=C_P25;
            if (strncmp(buffer, "[NXDN]", 6) == 0)               ok=C_NXDN;
            if (strncmp(buffer, "[POCSAG]", 6) == 0)             ok=C_POCSAG;
            if (strncmp(buffer, "[D-Star Network]", 16) == 0)    ok=C_DSTARNET;
            if (strncmp(buffer, "[DMR Network]", 13) == 0)       ok=C_DMRNET;
            if (strncmp(buffer, "[System Fusion Network]", 23) == 0) ok=C_YSFNET;
            if (strncmp(buffer, "[P25 Network]", 13) == 0)       ok=C_P25NET;
            if (strncmp(buffer, "[NXDN Network]", 14) == 0)      ok=C_NXDNNET;
            if (strncmp(buffer, "[Info]", 6) == 0)               ok=C_INFO;
            if (strncmp(buffer, "[Nextion]", 9) == 0)            ok=C_NEXTION;
            if (strncmp(buffer, "[Log]", 5) == 0)                ok=C_LOG;
            if (strncmp(buffer, "[Transparent Data]", 18) == 0)  ok=C_TRANSPARENT;
            if (strncmp(buffer, "[NextionDriver]", 15) == 0)     ok=C_NEXTIONDRIVER;
        }
        char* key   = strtok(buffer, " \t=\r\n");
        if (key == NULL)
            continue;

        char* value = strtok(NULL, "\r\n");
        if (value == NULL)
            continue;

        //1-20 = modes enabled/disabled
        if (ok<21) {
            if (strcmp(key, "Enable") == 0) {
                modeIsEnabled[ok] = (unsigned int)atoi(value);
                found++;
            }
        }

        if (ok==C_INFO) {
            if (strcmp(key, "RXFrequency") == 0) {
                RXfrequency = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"Found RX Frequency %d",RXfrequency);
                found++;
            }
            if (strcmp(key, "TXFrequency") == 0) {
                TXfrequency = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"Found TX Frequency %d",TXfrequency);
                found++;
            }
            if (strcmp(key, "Location") == 0) {
                remove_all_chars(value,'"');
                strcpy(location,value);
                writelog(LOG_NOTICE,"Found Location [%s]",location);
                found++;
            }
        }
        if (ok==C_NEXTION) {
            if (strcmp(key, "Port") == 0) {
                strcpy(nextionDriverLink,value);
                writelog(LOG_NOTICE,"Found Virtual Port [%s]",nextionDriverLink);
                found++;
            }
            if (strcmp(key, "ScreenLayout") == 0) {
                screenLayout = (unsigned int)atoi(value);
                found++;
            }
            if (strcmp(key, "DisplayTempInFahrenheit") == 0) {
                tempInF = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"Showing temperature in %s", (tempInF==0) ? "Celsius" : "Farenheit" );
                found++;
            }
        }
/*
        if (ok==C_LOG) {
            if (strcmp(key, "FileLevel") == 0)
                loglevel = (unsigned int)atoi(value);
        }
*/

        if (ok==C_TRANSPARENT) {
            if (strcmp(key, "Enable") == 0) {
                transparentIsEnabled = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"Use Transparent Connection: %s", (transparentIsEnabled==0) ? "NO" : "YES");
                found++;
            }
            if (strcmp(key, "LocalPort") == 0) {
                strcpy(remotePort,value);    //yes! the local port from MMDVMHost is our remote port !
                writelog(LOG_NOTICE,"  Remote port: %s", remotePort);
                found++;
            }
            if (strcmp(key, "RemotePort") == 0) {
                strcpy(localPort,value);    //yes! the remote port from MMDVMHost is our local port !
                writelog(LOG_NOTICE,"  Local port: %s", localPort);
                found++;
            }
            if (strcmp(key, "SendFrameType") == 0) {
                sendFrameType = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"  Send Frame Type: %s", (sendFrameType==0) ? "NO" : "YES");
                found++;
            }
        }

        if (ok==C_NEXTIONDRIVER) {
            if (strcmp(key, "Port") == 0) {
                strcpy(nextionPort,value);
                writelog(LOG_NOTICE,"Found Nextion Port [%s]",nextionPort);
                found++;
            }
            if (strcmp(key, "LogLevel") == 0) {
                verbose=(unsigned int)atoi(value);
                if (verbose<0) verbose=0;
                if (verbose>4) verbose=4;
                found++;
            }
            if (strcmp(key, "DataFilesPath") == 0) {
                strcpy(datafiledir,value);
                if (datafiledir[strlen(datafiledir)-1]!='/') {
                    strcat(datafiledir,"/");
                }
                found++;
            }
            if (strcmp(key, "GroupsFile") == 0) {
                strcpy(groupsFile,value);
                found++;
            }
            if (strcmp(key, "GroupsFileSrc") == 0) {
                strncpy(groupsFileSrc,value,sizeof(groupsFileSrc)-1);
                groupsFileSrc[sizeof(groupsFileSrc)-1]=0;
                writelog(LOG_NOTICE,"Groups file will be fetched from [%s]",groupsFileSrc);
                found++;
            }
            if (strcmp(key, "DMRidFile") == 0) {
                strcpy(usersFile,value);
                found++;
            }
            if (strcmp(key, "DMRidFileSrc") == 0) {
                strncpy(usersFileSrc,value,sizeof(usersFileSrc)-1);
                usersFileSrc[sizeof(usersFileSrc)-1]=0;
                writelog(LOG_NOTICE,"Users file will be fetched from [%s]",usersFileSrc);
                found++;
            }
            //----- Users -----
            if (strcmp(key, "DMRidDelimiter") == 0) {
                userDBDelimiter=value[0];
                found++;
            }
            if (strcmp(key, "DMRidId") == 0) {
                userDBId=(unsigned int)atoi(value);
                if ((userDBId<1)||(userDBId>10))userDBId=1;
                userDBId--;
                found++;
            }
            if (strcmp(key, "DMRidCall") == 0) {
                userDBCall=(unsigned int)atoi(value);
                if ((userDBCall<1)||(userDBCall>10))userDBCall=2;
                userDBCall--;
                found++;
            }
            if (strcmp(key, "DMRidName") == 0) {
                if (value[1]==',') { value[1]=value[2]; value[2]=0; }
                userDBName=(unsigned int)atoi(value);
                if ((userDBName<1)||(userDBName>99))userDBName=34;
                found++;
            }
            if (strcmp(key, "DMRidX1") == 0) {
                userDBX1=(unsigned int)atoi(value);
                if ((userDBX1<1)||(userDBX1>10))userDBX1=5;
                userDBX1--;
                found++;
            }
            if (strcmp(key, "DMRidX2") == 0) {
                userDBX2=(unsigned int)atoi(value);
                if ((userDBX2<1)||(userDBX2>10))userDBX2=6;
                userDBX2--;
                found++;
            }
            if (strcmp(key, "DMRidX3") == 0) {
                userDBX3=(unsigned int)atoi(value);
                if ((userDBX3<1)||(userDBX3>10))userDBX3=7;
                userDBX3--;
                found++;
            }
            //----- ----- -----
            if (strcmp(key, "ChangePagesMode") == 0) {
                changepages = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"ChangePagesMode: %s", changepages ? "ON" : "OFF");
                found++;
            }
            if (strcmp(key, "RemoveDim") == 0) {
                removeDim = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"RemoveDim: %s", removeDim ? "ON" : "OFF");
                found++;
            }
            if (strcmp(key, "SleepWhenInactive") == 0) {
                sleepWhenInactive = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"SleepWhenInactive: %s", sleepWhenInactive ? "ON" : "OFF");
                found++;
            }

            if (strcmp(key, "ShowModesStatus") == 0) {
                showModesStatus = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"ShowModesStatus: %s", showModesStatus ? "ON" : "OFF");
                found++;
            }
            if (strcmp(key, "SendUserDataMask") == 0) {
                int base; if ((value[1]!=0)&&(value[1]=='b')) { base=2; value[0]=' ', value[1]=' '; } else { base=0; }
                sendUserDataMask = (unsigned int)strtol(value, NULL, base);
                writelog(LOG_NOTICE,"SendUserDataMask: %d %s | 0x%X", base, value, sendUserDataMask);
                found++;
            }
            if (strcmp(key, "WaitForLan") == 0) {
                waitForLan = (unsigned int)atoi(value);
                writelog(LOG_NOTICE,"WaitForLan: %s", waitForLan ? "ON" : "OFF");
                found++;
            }
        }
    }
    fclose(fp);

    if (found == 0) {
        writelog(LOG_ERR,"Found no data in config file %s.\n", configFile);
    }
    return found;
}


void readVersions(char *filename) {
  char buffer[102], *val;

//  printf("--- File  [%s]\n",filename);
  FILE* fp = fopen(filename, "rt");
  if (fp != NULL) {
    while (fgets(buffer, 100, fp) != NULL) {
        sanitize(buffer);
        val=strstr(buffer,"=");
        if (val==NULL) continue;
        val[0]=0; val++;
//        printf("Key [%s]\n",buffer);
//        printf("Val [%s]\n\n",val);
        if (strcmp(buffer,"PRETTY_NAME")==0) { strcpy(OSname,val); }
        if (strcmp(buffer,"Version")==0)     { strcpy(PIname,val); }
    }
    fclose(fp);
  }
}


int getDiskFree(int log){
  struct statfs sStats;
  char fname[250];
  int ok;

  strcpy(fname,datafiledir);
  strcat(fname,groupsFile);

  ok = statfs( fname, &sStats );
  if (ok==-1) {
    writelog(LOG_ERR,"No groups file found (%s), trying config file to calculate disk size",fname);
    ok = statfs( configFile1 , &sStats );
  }

  if( ok == -1 ) {
      writelog(LOG_ERR,"Unable to find any file to calculate disk size");
      return -1;
  } else {
    int size,free;
    //sizes in MB;
    size=((sStats.f_blocks/1024)*sStats.f_bsize)/1024;
    free=((sStats.f_bavail/1024)*sStats.f_bsize)/1024;
    if (log) {
        writelog(LOG_NOTICE,"Disk size : %d MB (%d free)",size,free);
    }
    // in PCT
    return (100*free)/size;
  }
}


int getHWinfo(char* info) {
    size_t n = 0;
    char *line = NULL;

    strcpy(info,"");
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp==NULL) return ENOENT;

    while (getline(&line, &n, fp) > 0) {
            if (strstr(line, "model name")) { sprintf(info,"%s",line); }
            if (strstr(line, "Model")) { sprintf(info,"%s",line); }
    }
    char *token;
    const char s[3] = ":\n";
    token = strtok(info, s);
    if ( token != NULL ) {
      token = strtok(NULL, s);
    }
    sprintf(info, "%s", token+1 );
    free(line);
    fclose(fp);
    return 0;
}

int getMeminfo(char* info) {
    size_t n = 0;
    char *line = NULL;
    int i;

    strcpy(info,"");
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp==NULL) return ENOENT;

    while (getline(&line, &n, fp) > 0) {
            if (strstr(line, "MemTotal")) { sprintf(info,"%s",line); }
    }
    char *token;
    const char s[3] = ":\n";
    token = strtok(info, s);
    if ( token != NULL ) {
      token = strtok(NULL, s);
    }
    for(i=0; i<20; i++) { if (token[0]==0x20) token++; else break; }
    sprintf(info, "%s", token );
    free(line);
    fclose(fp);
    return 0;
}



#define LH_PAGES 	7
#define LH_INDEXES 	20
#define LH_FIELDS 	40


char data[LH_PAGES][LH_FIELDS][LH_INDEXES][100];
unsigned char startdata[LH_PAGES];
unsigned char LHinhibit;

void addLH(char* displaydatabuf ) {
    int i,f,field, test1, test2;
    char* split;
    char displaydata[1024];

    if (displaydatabuf==NULL) return;
    if ((displaydatabuf[0]=='L')&&(displaydatabuf[1]=='H'))return;
    strcpy(displaydata,displaydatabuf);
    displaydata[99]=0;
    writelog(LOG_DEBUG,"  LH: check [%s] on page %d",displaydata,page);
    if ((page<0)||(page>=LH_PAGES)) { writelog(LOG_ERR,"LH: nonexistent page"); return; }

    split=strstr(displaydata,"=");
    if ((split!=NULL)&&(strstr(displaydata,"MMDVM.status.val")!=NULL)) {
        split[0]=' ';
        statusval=atoi(split);
        return;
    }

    split=strstr(displaydata,".txt=");
    if (split==NULL) { return; }
    split[0]=0;
    i=strlen(displaydata);
    while(i>0) {i--; if ((displaydata[i]<'0')||(displaydata[i]>'9'))displaydata[i]=' '; }
    field=atoi(displaydata);
    split++;
    strncpy(data[page][field][0],split,99);
    writelog(LOG_DEBUG,"  LH: page %d field %d is [%s]",page,field,split);
    writelog(LOG_DEBUG,"  LH: statusval %d",statusval);

/*
     42 : Dstar type/my1/my2
     62 : DMR ID1
     70 : DMR ID2
     82 : YSF src
    102 : P25 source
    122 : NXDN source
    132 : POCSAG RIC
*/

    test1=((statusval==42)||(statusval==62)||(statusval==70)||(statusval==82)||(statusval==102)||(statusval==122)||(statusval==132));
    if (test1) { writelog(LOG_DEBUG,"NO LH Inhibit\n "); LHinhibit=0; }
/*
     41 : D-Star listening
     61 : DMR listening1
     64 : DMR Call end1
     69 : DMR listening2
     72 : DMR Call end2
     81 : YSF listening
    101 : P25 listening
    121 : NXDN listening
*/
    test2=((statusval==41)||(statusval==61)||(statusval==69)||(statusval==81)||(statusval==101)||(statusval==121)||(statusval==131)
            ||(statusval==64)||(statusval==72)
            );
    if (test2||LHinhibit) { writelog(LOG_DEBUG,"LH Inhibit\n "); LHinhibit=1; return; }

    if (startdata[page]==0) startdata[page]=1;
//printf("===page %d field %d index %d current[%s] split[%s] test[%c]\n",page,field,startdata[page],data[page][field][startdata[page]],split,test1?'Y':'N');
    if ( (page==0)||(test1) ) {
        startdata[page]++;
//												if (page>0)printf("-----------New index !-----------\n");
    }

    if (startdata[page]>=LH_INDEXES) startdata[page]=1;
    i=startdata[page];
    if ((page==0)||(test1)){ for (f=0;f<LH_FIELDS;f++) strncpy(data[page][f][i],data[page][f][0],99); strcpy(data[page][f][0],""); statusval=0; }
    strncpy(data[page][field][i],split,99);
//printf("===write page%d field %d index %d split[%s] \n",page,field,i,split);

//for (f=0;f<LH_FIELDS;f++) if (strlen(data[page][f][i])>0)printf("page %d index %d veld %d [%s]\n",page,i,f,data[page][f][i]);
}


void LHlist(int page,int aant) {
    int i,f,s;
    char text[100],tmp[50];

//nog niet OK
    if (aant>=LH_INDEXES)aant=LH_INDEXES;
    writelog(LOG_DEBUG,"Last Heard: page %d, %d lines",page,aant);

    for (i=0;i<aant;i++){
        s=startdata[page]-i; if (s<1) s+=LH_INDEXES;
//printf("--> %d --> %d  \n",i,s);
        for (f=0;f<LH_FIELDS;f++) {
            if (strlen(data[page][f][s])>1) {
                strncpy(tmp,data[page][f][s],48); tmp[49]=0;
                sprintf(text,"LH%dt%d.%s",i,f,tmp);
                sendCommand(text); usleep(50);
            }
        }
    }
}



void dumpLHlist(void) {
    int p,i,f;

//    for (p=0;p<LH_PAGES;p++) {
p=2;{
        printf("---------------------PAGE %d--(start %d)-------------------\n",p,startdata[p]);
        for (i=0;i<LH_INDEXES;i++){
            printf("Index %d : ",i);
            for (f=0;f<LH_FIELDS;f++) {
                if (strlen(data[p][f][i])>6) printf("(%d)t%d [%s]  ",startdata[p],f,data[p][f][i]);
            }
            printf("\n");
        }
    }
}


void sendScreenData(unsigned int pagenr) {
    int field;
    char text[100];

    if (pagenr==0xFE) pagenr=0;
    if (pagenr>LH_PAGES) { writelog(LOG_ERR,"Refresh Screen: nonexistent page"); return; }

    writelog(LOG_DEBUG,"Refresh Screen: sending fields for page %d",pagenr);
    for (field=0; field<LH_FIELDS; field++){
        sprintf(text,"t%d.%s",field,data[pagenr][field][startdata[page]]);
        if (strlen(data[pagenr][field][startdata[page]])>0) {
            writelog(LOG_DEBUG," LH: sending %s",text);
            sendCommand(text);
        }
    }
    writelog(LOG_DEBUG,"Refresh Screen: sending fields done");
}



int search_group(int nr, group_t a[], int m, int n)
{
    writelog(LOG_DEBUG,"--- Group search for ID %d (%d - %d)",nr,m,n);
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


int search_user_index_for_ID(int nr, user_t a[], int m, int n)
{
    writelog(LOG_DEBUG,"--- User search for ID %d (%d [%d] - %d [%d] )",nr,m,a[m].nr,n,a[n].nr);
//    usleep(200000);
    if (nr==0) return -1;
    if (m>n) return -1;

    if((n-m)<2) {
        if(a[n].nr == nr) { return n; } else  return -1;
    }

    int middle=(m+n)/2;
    if(a[middle].nr==nr)  return middle;
    else
    if(nr > a[middle].nr) return search_user_index_for_ID(nr, a, middle, n);
    else
    if(nr < a[middle].nr) return search_user_index_for_ID(nr, a, m, middle);

    return -1;
}


int search_user_array_for_CALL(char* call, user_call_idx_t a[], int m, int n)
{
    writelog(LOG_DEBUG,"--- User index search for call %s (%d [%s] - %d [%s] )",call,m,a[m].call,n,a[n].call);

    if (strlen(call)==0) return -1;
    if (m>n) return -1;

    if((n-m)<2) {
        if(strcmp(a[n].call,call)==0) { return n; } else  return -1;
    }

    int middle=(m+n)/2;
    if(strcmp(a[middle].call,call)==0)  return middle;
    else
    if(strcmp(a[middle].call,call)<0) return search_user_array_for_CALL(call, a, middle, n);
    else
    if(strcmp(a[middle].call,call)>0) return search_user_array_for_CALL(call, a, m, middle);

    return -1;
}


int search_user_index_for_CALL(char* call, user_call_idx_t a[], int m, int n){
    int i;

    i=search_user_array_for_CALL(call, a,  m,  n);
    if (i>0) return usersCALL_IDX[i].nr; else return -1; //KE7FNS  Should return an error, not the first index in the table
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

void print_call_id() {
    int i;

    for (i=0; i<nmbr_users;i++)
        printf("User %15s: %d\n", usersCALL_IDX[i].call, usersCALL_IDX[i].nr);
}





int insert_user_call_idx(user_call_idx_t table[], int index, void *call, int nr){
    char *m;

    if (index>=MAXUSERS) {
        writelog(LOG_ERR,"  Maximum of %d users reached. Not adding more users.",index);
        return 0;
    }

    m = malloc(256);
    if (m==NULL) return 0;
    free(m);

    table[index].nr = nr;

    int size;
    size=strlen(call)+1;
    table[index].call = malloc(size);
    memcpy(table[index].call,call,size);

//	writelog(LOG_DEBUG,"Inserted call [%s] = index [%d]",table[index].call,table[index].nr);

    return 1;
}


int insert_user(user_t table[], int nr, void *new_data1, void *new_data2, void *new_data3, void *new_data4, void *new_data5)
{
    char *m;

    if (nmbr_users>=MAXUSERS) {
        writelog(LOG_ERR,"  Maximum of %d users reached. Not adding more users.",nmbr_users);
        return 0;
    }

    m = malloc(1024);
    if (m==NULL) return 0;
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

//printf("User index %5d: %d = [%s][%s][%s][%s][%s]\n", nmbr_users, table[nmbr_users].nr, table[nmbr_users].data1, table[nmbr_users].data2, table[nmbr_users].data3, table[nmbr_users].data4, table[nmbr_users].data5);

    nmbr_users++;

    return 1;
}



int insert_group(group_t table[], int nr, void *new_data){
    char *m;

    if (nmbr_groups>=MAXGROUPS) {
        writelog(LOG_ERR,"  Maximum of %d groups reached. Not adding more groups.",nmbr_groups);
        return 0;
    }

    m = malloc(256);
    if (m==NULL) return 0;
    free(m);

    table[nmbr_groups].nr = nr;

    int size;
    size=strlen(new_data)+1;
    table[nmbr_groups].name = malloc(size);
    memcpy(table[nmbr_groups].name,new_data,size);
    nmbr_groups++;

    return 1;
}



int cmpstringp( const void* a, const void* b)
{
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );

     return strcmp(users[int_a].data1,users[int_b].data1);
}


void readGroups(void){
#define BUFFER_SZ 100
    char fname[250];

    strcpy(fname,datafiledir);
    strcat(fname,groupsFile);

    writelog(LOG_NOTICE,"  Reading groups from %s",fname);

    nmbr_groups=0;

    FILE* fp = fopen(fname, "rt");
    if (fp == NULL) {
        writelog(LOG_WARNING, " ERROR: Couldn't open the groups file %s.",fname);
        return;
    }

    char buffer[BUFFER_SZ];
    int nr;
    insert_group(groups, 0, "");
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
        if (insert_group(groups, nr, key)==0) break;
    }
    fclose(fp);
    writelog(LOG_NOTICE,"  Read %d groups.",nmbr_groups);
}


void convertStoDHMS(long int age,char * DHMS) {
    int temp;

    if (age>INT_MAX) age=INT_MAX;
    temp=age/86400;
    sprintf(DHMS,"%d days",temp);
    if (temp>2) return;
    temp=age/3600;
    sprintf(DHMS,"%d hour",temp);
    if (temp>2) return;
    temp=age/60;
    sprintf(DHMS,"%d min",temp);
    if (temp>2) return;
    sprintf(DHMS,"%d s",temp);
}



void updateDB(int age) {
    struct stat attr;
    char fname[250], cmd[550],text[120],tmp[100],dt[50],unit[20];
    int ok;

    time_t nu = time(NULL);
    tmp[0]=0;

    strcpy(fname,datafiledir);
    strcat(fname,groupsFile);
    ok=stat(fname, &attr);
    if (ok!=0) {
        sprintf(tmp," No Groups file found");
        writelog(LOG_ERR,tmp);
    } else {
        strftime(dt, 50, "%Y-%m-%d %H:%M:%S", localtime(&attr.st_mtime));
        convertStoDHMS(nu-attr.st_mtime,unit);
        sprintf(tmp," Groups file : %s (%s old)",dt,unit);
        writelog(LOG_NOTICE,tmp);
    }
    if (age>0) {
      if ((ok!=0)||(nu-attr.st_mtime>age)) {
        sprintf(text, "msg1.txt=\"Updating ...\"");
        sendCommand(text);
        if (strcmp(groupsFileSrc,GROUPSFILESRC)!=0) writelog(LOG_NOTICE," Fetching groups from %s",groupsFileSrc);
        sprintf(cmd, "wget %s -O /tmp/groups >/dev/null 2>&1 && touch /tmp/groups && mv /tmp/groups %s",groupsFileSrc,fname);
        ok=system(cmd);
        if (ok!=0) {
            sprintf(text, "msg.txt=\"Groups file update failed.\"");
            sendCommand(text);
            sprintf(tmp," ERROR: Groups file update failed (%d)",ok);
            writelog(LOG_ERR,tmp);
        } else {
            sprintf(tmp," Groups file downloaded");
            writelog(LOG_NOTICE,tmp);
        }
      } else {
        sprintf(text, "msg.txt=\"Files too young. Not updating.\"");
        sendCommand(text);
      }
    }
    sprintf(text, "msg1.txt=\"%s\"",tmp);
    if (age<3000000) sendCommand(text);

    strcpy(fname,datafiledir);
    strcat(fname,usersFile);
    ok=stat(fname, &attr);
    if (ok!=0) {
        sprintf(tmp," No Users file found");
        writelog(LOG_ERR,tmp);
    } else {
        strftime(dt, 50, "%Y-%m-%d %H:%M:%S", localtime(&attr.st_mtime));
        convertStoDHMS(nu-attr.st_mtime,unit);
        sprintf(tmp," Users file : %s (%s old)",dt,unit);
        writelog(LOG_NOTICE,tmp);
    }
    if (age>0)
      if ((ok!=0)||(nu-attr.st_mtime>age)) {
        if (strcmp(usersFileSrc,USERSFILESRC)!=0) writelog(LOG_NOTICE," Fetching users from %s",groupsFileSrc);
        sprintf(cmd, "wget %s -O /tmp/users >/dev/null >/dev/null 2>&1 && touch /tmp/users && mv /tmp/users %s",usersFileSrc,fname);
        ok=system(cmd);
        if (ok!=0) {
            sprintf(tmp," ERROR: Users file update failed (%d)",ok);
            writelog(LOG_ERR,tmp);
        } else {
            sprintf(tmp," Users file downloaded");
            writelog(LOG_NOTICE,tmp);
        }
    }
    sprintf(text, "msg2.txt=\"%s\"",tmp);
    if (age<3000000) sendCommand(text);
}




void readUserDB(void){
#define BUFFER_SZ 100
    char fname[250];
    char delimiters[3];
    int i,nr;
    char buffer[BUFFER_SZ];
    char username[100];
    char *key[10],*next,*bufP;
    char *niks = "";

    strcpy(fname,datafiledir);
    strcat(fname,usersFile);
    writelog(LOG_NOTICE,"  Reading users from %s",fname);
    writelog(LOG_NOTICE,"   delimiter '%c'",userDBDelimiter);
    writelog(LOG_NOTICE,"   DMRid in field %d",userDBId+1);
    writelog(LOG_NOTICE,"   Call  in field %d",userDBCall+1);
    if (userDBName>10) {
        writelog(LOG_NOTICE,"   Name  in fields %d + %d",(userDBName/10),(userDBName%10));
    } else {
        writelog(LOG_NOTICE,"   Name  in field %d",userDBName);
    }
    writelog(LOG_NOTICE,"   Extra data in fields %d,%d and %d",userDBX1+1,userDBX2+1,userDBX3+1);

    sprintf(delimiters,"%c\n",userDBDelimiter);
    nmbr_users=0;

    FILE* fp = fopen(fname, "rt");
    if (fp == NULL) {
        writelog(LOG_WARNING, " ERROR: Couldn't open the userDB file %s.",fname);
        return;
    }

    insert_user(users, 0, "","","","","");

    struct timespec start, stop;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    while (fgets(buffer, BUFFER_SZ, fp) != NULL) {
        buffer[strlen(buffer)-1]=0;
        sanitize(buffer);
        bufP=&buffer[0];
//    printf("Analysing %s\n",buffer);
        for(i=0;i<10;i++)key[i]=niks;
        next = strsep(&bufP, delimiters);
        if (next == NULL) continue;
        i=0;
        while((i<10)&&(next != NULL))
        {
            key[i++]=next;
            next = strsep(&bufP,delimiters);
        }
        nr=atoi(key[userDBId]);
        if ( (nr>1000000)&&(nr<10000000)&&(strlen(key[userDBCall])>3) ) {
            if (userDBName>10) { 
                sprintf(username,"%s %s",key[(userDBName/10)-1],key[(userDBName%10)-1]); 
            } else {
                sprintf(username,"%s",key[userDBName-1]);
            }
//printf("%5d Pushing [%d] [%s] [%s] [%s] [%s]  [%s]\n",nmbr_users, nr,  key[userDBCall], username, key[userDBX1], key[userDBX2], key[userDBX3]);fflush(NULL);
//usleep(100000);
            if (insert_user(users, nr, key[userDBCall], username, key[userDBX1], key[userDBX2], key[userDBX3])==0) break;
        }
//            else printf("   Not inserting [%d][%s]\n",nr,key[userDBCall]);

    }
    fclose(fp);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
    double result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;
    writelog(LOG_NOTICE,"  Read %d users in %0.0f ms.",nmbr_users-1,result);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    int userindexes[MAXUSERS];
    for (i=0;i<nmbr_users;i++) userindexes[i]=i;
    qsort( userindexes, nmbr_users, sizeof(userindexes[0]), cmpstringp );
    for (i=0;i<nmbr_users;i++){ insert_user_call_idx(usersCALL_IDX,i,users[userindexes[i]].data1, userindexes[i]); }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
    result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;
    writelog(LOG_NOTICE,"  Sorted CALL table in %0.0f ms.",result);
}


void readUsersGroups(void) {
    writelog(LOG_NOTICE,"Clearing users and groups ...");
    int i;

    for (i=nmbr_groups-1;i>=0;i--) {
        free(groups[i].name);
    }
    for (i=nmbr_users-1;i>=0;i--) {
        free(users[i].data1);
        free(users[i].data2);
        free(users[i].data3);
        free(users[i].data4);
        free(users[i].data5);
        free(usersCALL_IDX[i].call);
    }

    writelog(LOG_NOTICE,"Re-reading users and groups ...");
    readGroups();
    readUserDB();
}


pid_t proc_find(const char* name)
{
    DIR* dir;
    struct dirent* ent;
    char buf[512];

    long  pid,mypid;
    char pname[100] = {0,};
    char state;
    FILE *fp=NULL; 

    if (!(dir = opendir("/proc"))) {
//        perror("can't open /proc");
        return -1;
    }
    mypid=getpid();

    while((ent = readdir(dir)) != NULL) {
        long lpid = atol(ent->d_name);
        if ((lpid < 0)||(lpid == mypid))
            continue;
        snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
        fp = fopen(buf, "r");

        if (fp) {
            if ( (fscanf(fp, "%ld (%[^)]) %c", &pid, pname, &state)) != 3 ){
                writelog(LOG_DEBUG,"fscanf failed");
                fclose(fp);
                closedir(dir);
                return -1; 
            }
            if (!strcmp(pname, name)) {
                fclose(fp);
                closedir(dir);
                return (pid_t)lpid;
            }
            fclose(fp);
        }
    }
closedir(dir);
return -1;
}



int openTalkingSocket(void) {
    const char* hostname="127.0.0.1"; /* localhost */
    struct addrinfo hints;

    if (transparentIsEnabled==0) return 0;

    memset(&hints,0,sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags    = AI_ADDRCONFIG;

    writelog(LOG_DEBUG,"Try to open socket to %s:%s", hostname,remotePort);
    int err=getaddrinfo(hostname,remotePort,&hints,&display_addr);
    if (err!=0) {
        writelog(LOG_ERR,"Transparent Connection: failed to resolve remote socket address (err=%d)",err);
        writelog(LOG_ERR,"  getaddrinfo: %s\n", gai_strerror(err));
        return 0;
    }

    display_TXsock=socket(display_addr->ai_family,display_addr->ai_socktype,display_addr->ai_protocol);
    if (display_TXsock==-1) {
        writelog(LOG_ERR,"Transparent Connection: %s",strerror(errno));
        return 0;
    }

    writelog(LOG_NOTICE,"Transparent Connection: talking socket open, fd=%d",display_TXsock);

    return 1;
}


int sendTransparentData(int display,char* msg) {

    if ((transparentIsEnabled==0)||(display_TXsock<1)) return 0;

    char content[100];

    content[0]=0x90;
    if (display==1) content[0]=0x80;

    writelog(LOG_DEBUG," NET: %s",msg);

    strncpy(&content[1],msg,95);
    content[95]=0;
    strcat(content,"\xff\xff\xff");

    if (sendto(display_TXsock,content,strlen(content),0, display_addr->ai_addr,display_addr->ai_addrlen)==-1) {
        writelog(LOG_ERR,"Transparent Send: %s",strerror(errno));
        return 0;
    } else 
    return 1;
}


int openListeningSocket(void) {
    int rv;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *addr;

    writelog(LOG_DEBUG,"Try to open listening socket %s", localPort);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, localPort, &hints, &servinfo)) != 0) {
        writelog(LOG_ERR, "getaddrinfo: %s", gai_strerror(rv));
        return 0;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        addr = (struct sockaddr_in *)p->ai_addr; 
        writelog(LOG_NOTICE,"Try to bind %s ...",inet_ntoa((struct in_addr)addr->sin_addr));

        if ((display_RXsock = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            writelog(LOG_ERR,"listener: socket");
            continue;
        }
        if (bind(display_RXsock, p->ai_addr, p->ai_addrlen) == -1) {
            close(display_RXsock);
            writelog(LOG_ERR,"listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        writelog(LOG_ERR, "listener: failed to bind socket");
        return 0;
    }
    fcntl(display_RXsock, F_SETFL, O_NONBLOCK);
    freeaddrinfo(servinfo);
    writelog(LOG_NOTICE,"Transparent Connection: listening socket open, fd=%d",display_RXsock);
    return 1;
}
