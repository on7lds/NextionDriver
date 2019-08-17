/*
 *   Copyright (C) 2017...2019 by Lieven De Samblanx ON7LDS
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <time.h>


#include "NextionDriver.h"
#include "basicFunctions.h"
#include "processCommands.h"
#include "processButtons.h"
#include "helpers.h"

const char ENDMRKR[3]="\xFF\xFF\xFF";
int RXtail=0;
char RXbuffertemp[1024];
int sockRXtail=0;
char sockRXbuffertemp[1024];


int open_nextion_serial_port(char* devicename, long BAUD);



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


void writelog(int level, char *fmt, ...)
{
    va_list args;
    char str[1024];

    va_start(args, fmt);
    vsnprintf(str, 1024, fmt, args);
    va_end(args);

    if ( (become_daemon==TRUE) && (verbose>(level-4)) ) {
        str[98]='.';str[99]='.';str[100]='.'; str[101]=0; 
            if (!((str[5]=='2')&&(str[20]==' ')&&(strlen(str)==30))) syslog(level, str);
    } else {
        printf("%s\n",str);
    }
}


void sendCommand(char *cmd){
    if (strlen(cmd)>0) {
        if (transparentIsEnabled==0)  //KE7FNS if we are using transparent data don't try to write to a serial port that isn't open.
        {
          write(fd2,cmd,strlen(cmd));
          write(fd2,"\xFF\xFF\xFF",3);
        }
        writelog(LOG_DEBUG,"  TX: %s",cmd);    //KE7FNS fix spacing in log
        if (screenLayout==4)
            usleep(1042*(strlen(cmd)+1));
        else
            usleep(87*(strlen(cmd)+1));

        sendTransparentData(MMDVM_DISPLAY,cmd);

        addLH(cmd);
    }
    if (!become_daemon)fflush(NULL);
}


int checkDisplay(char *model) {
    char buffer[200];
    char* p;
    int i,r,d;
    int flashsize=0;
    r = read (fd2,&buffer,200);
    d=0;

    if (strcmp(nextionPort,"modem")==0) {
        writelog(LOG_NOTICE,"I can not (yet) check or update modem connected displays");
        return -1;
    }
    for (i=0;i<8;i++)DisplayInfo[i][0]=0;

    strcpy(model,"");
    for (i=0; i<3; i++) {
        writelog(LOG_DEBUG,"Checking display ...");
        write(fd2,"\xFF\xFF\xFF",3); write(fd2,"connect",7); write(fd2,"\xFF\xFF\xFF",3); usleep(134000);
        r = read (fd2,&buffer[d],200);
        if (r>0) { buffer[r]=0; d+=r; }
        //comok <touch(1)/notouch(0)>,reserved,model,fw ver,mcu,s/n,flash size
        if (r>0) {
            //writelog(LOG_DEBUG,"Received %d bytes",r);
            p=strstr(buffer,ENDMRKR);
            if (p==NULL) { usleep (200000); continue; } else { p[0]=0; }
            p=strstr(buffer,"comok");
            r=0;
            if (p!=NULL) {
                strcpy(DisplayInfo[r],p);
                p = strtok (buffer,",");
                while (p != NULL) {
                    if (r<8) r++;
                    strcpy(DisplayInfo[r],p);
                    //printf ("%d: %s\n",r,p);
                    p = strtok (NULL, ",");
                }
                if (r==7) {
                    writelog(2,"Found Nextion display");
                    writelog(2," %s display model %s",DisplayInfo[1][6]=='1' ? "Touch" : "No touch",DisplayInfo[3]);
                    writelog(2," FW %s, MCU %s",DisplayInfo[4],DisplayInfo[5]);
                    writelog(2," Serial %s",DisplayInfo[6]);
                    writelog(2," Flash size %s",DisplayInfo[7]);

                    flashsize=atoi(DisplayInfo[7]);
                    p=strstr(DisplayInfo[3],"_"); if (p!=NULL) p[0]=0;
                    strcpy(model,DisplayInfo[3]);

                    break;
                }
            }
        }
        sleep(1);
    }
    return flashsize;
}


void updateDisplay(void) {
    int flashsize,filesize,ok,i,r,baudrate,blocks;
    char model[50];
    char buffer[4096],bestand[1024];
    struct dirent *de;
    time_t tijd;
    FILE *ptr;

    if (screenLayout==4) {
            baudrate=115200;
        } else {
            baudrate=9600;
        }

    writelog(2,"Trying to update display ...");
    flashsize=checkDisplay(model);
    strcat(model,".tft");
    if (flashsize==0) 
        { writelog(LOG_ERR,"Could not communicate with display. Cannot update ..."); return; }
    if (flashsize<0) 
        { writelog(LOG_ERR,"Cannot update modem connected displays ..."); return; }
    //search model file
    ok=0;
    DIR *dr = opendir(datafiledir);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
    {
        writelog(LOG_ERR,"Could not open DataFilesPath directory" );
        return;
    }
    while (((de = readdir(dr)) != NULL)&&(!ok))
        if (strcasecmp(de->d_name,model)==0) {
            writelog(LOG_DEBUG,"FOUND %s", de->d_name);
            strcpy(model,de->d_name);
            ok=1;
        }
    closedir(dr);
    if (ok==0) 
        { writelog(LOG_ERR,"Could not find display update file (%s)",model ); return; }
    sprintf(bestand,"%s/%s",datafiledir,model);
    ptr = fopen(bestand,"rb");
    if (ptr==NULL) 
        { writelog(LOG_ERR,"Could not open file %s",model); return; }
    //file size ? 
    fseek(ptr, 0L, SEEK_END);
    filesize = ftell(ptr);
    if (filesize<1) 
        { writelog(LOG_ERR,"Unable to get TFT file size"); return; }
    //not to big for display flash ?
    if (filesize>flashsize)
        { writelog(LOG_ERR,"TFT file to big for flash"); return; }
    blocks=filesize/4096;
    writelog(LOG_DEBUG,"I will write %d blocks",blocks);
    //start update
    //https://www.itead.cc/blog/nextion-hmi-upload-protocol
    sprintf(buffer,"whmi-wri %d,%d,0",filesize,baudrate);

    write(fd2,buffer,strlen(buffer));write(fd2,"\xFF\xFF\xFF",3);
    usleep(500000);
    r = read (fd2,&buffer,500);
    //display ready ?
    if (strstr(buffer,"\0x05")==NULL)
        { writelog(LOG_ERR,"No response from display to start  update"); return; }
    //Go !
    writelog(2,"Start update ...");
    writelog(2," %s will use %d%%%% of the flash",model,((filesize*100)/flashsize));

    tijd=time(NULL);
    rewind(ptr);
    r=fread(buffer,1,4096,ptr);
    while (r>0) {
        ok=write(fd2,buffer,r);
        i=50;
        ok=0;
        while (!ok && i) {
            usleep(100000);
            r=read(fd2,buffer,500);
            if ((r>0)&&(strstr(buffer,"\0x05")!=NULL)) ok=1; else i--;
        }
        if (ok==0)
            { writelog(LOG_ERR,"No response from display"); fclose(ptr); return; }
        r=fread(buffer,1,4096,ptr);
    }
    fclose(ptr);
    tijd=time(NULL)-tijd;
    writelog(2,"Update done in %d secs.",tijd);

}


void handleButton(int received) {
    char code, text[150], *cmd;
    int response,resNmbr;
    FILE *ls;

    response=0;
    if (received>1) {
        {
            sprintf(text,"RX: %d (",received);
            int i;
            for (i=0; i<received; i++) {
                sprintf(&text[strlen(text)], "%02X ",RXbuffer[i]);
            }
            sprintf(&text[strlen(text)], ")");
            writelog(LOG_DEBUG,text);
        }
        if (RXbuffer[0]==42) {
            writelog(LOG_NOTICE,"Received command 0x%02X",RXbuffer[1]);
            if (RXbuffer[1]>0xEF) {
                if ((RXbuffer[1]==0xFE)&&(received==3)){
                    sendScreenData(RXbuffer[2]);
                } else 
                if ((RXbuffer[1]==0xFD)&&(received==4)){
                    LHlist(RXbuffer[2],RXbuffer[3]);
                } else
                if ((RXbuffer[1]==0xFC)&&(received==3)){
                    if (RXbuffer[2]==2) {
                        if (DisplayInfo[3][0]==0) {
                            sprintf(text, "msg1.txt=\"Model Unknown\"");
                            sendCommand(text);
                            sprintf(text, "msg2.txt=\"\"");
                            sendCommand(text);
                        } else {
                            sprintf(text, "msg1.txt=\"%s\"",DisplayInfo[3]);
                            sendCommand(text);
                            sprintf(text, "msg2.txt=\"%s\"",DisplayInfo[6]);
                            sendCommand(text);
                        }
                    } else {
                        inhibit=RXbuffer[2];
                        if (inhibit==0xFE) inhibit=0;
                        writelog(LOG_DEBUG,"Inhibit=%d",inhibit);
                    }
                    response=RXbuffer[1];
                } else
                if (RXbuffer[1]==0xFB){
                    updateDisplay();
                } else
                if (RXbuffer[1]==0xF2){
                    dumpLHlist();
                } else {
                if ((RXbuffer[1]<0xF2)&&(received>2)&&(received<200)) {
                        cmd=&RXbuffer[2];
                        resNmbr=RXbuffer[2];
                        if (resNmbr<32) {
                            cmd++;
                            writelog(LOG_DEBUG," Command retunr field will be %d",resNmbr);
                        }
                        writelog(LOG_NOTICE," Execute command \"%s\"",cmd);
                        sprintf(TXbuffer, "msg.txt=\"Execute %s\"", cmd);
                        sendCommand(TXbuffer);
                        if (RXbuffer[1]==0xF1) {
                            ls = popen(cmd, "r");
                            char buf[256];
                            if (fgets(buf, sizeof(buf), ls) != 0) {
                                strtok(buf, "\n");
                                writelog(LOG_NOTICE," Command response \"%s\"",buf);
                                if (resNmbr>=32) {
                                    sprintf(TXbuffer, "msg.txt=\"%s\"", buf);
                                } else {
                                    sprintf(TXbuffer, "result%02d.txt=\"%s\"",resNmbr, buf);
                                }
                                sendCommand(TXbuffer);
                            }
                            pclose(ls);
                        } else {
                            system(&RXbuffer[2]);
                        }
                        response=RXbuffer[1];
                    }
                }
            } else {
                code=RXbuffer[1];
                memmove(&RXbuffer,&RXbuffer[2],512);
                writelog(LOG_NOTICE," Command parameter \"%s\"",RXbuffer);
                processButtons(code);
            }
        }
        if (response>0) {
            sprintf(text, "MMDVM.status.val=200");
            sendCommand(text);
            sendCommand("click S0,1");
        };

    }
}



void checkSerial(void) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd2, &fds);

    struct timeval t = {0, 100000};
    int sel=select(fd2 + 1, &fds, NULL, NULL, &t);
    if (sel<=0) return;

    char* s;
    int i;

    int r = read (fd2,&RXbuffertemp[RXtail],512);
    if (r>0) {
//        writelog(LOG_DEBUG,"Receive %d bytes from serial",r);
        for(i=0;i<r;i++) if (RXbuffertemp[RXtail+i]==0) RXbuffertemp[RXtail+i]=0xFE;

        RXtail+=r;
        RXbuffertemp[RXtail]=0;

        s=strstr(RXbuffertemp,ENDMRKR);
        while (s!=NULL) {
//            writelog(LOG_DEBUG,"Endmarker found");
            s[0]=0;
            memcpy(RXbuffer,RXbuffertemp,strlen(RXbuffertemp)+1);
            handleButton(strlen(RXbuffertemp));
            RXtail-=(strlen(RXbuffertemp)+3);
            memmove(&RXbuffertemp,&s[3],512);
            s=strstr(RXbuffertemp,ENDMRKR);
        }
    }
    if (RXtail>100) RXtail=0;
}



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void checkListeningSocket(void) {
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[1000];
    socklen_t addr_len;
    char a[INET6_ADDRSTRLEN];
    char* s;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(display_RXsock, &fds);

    struct timeval t = {0, 1000};
    int sel=select(display_RXsock + 1, &fds, NULL, NULL, &t);
    if (sel<=0) return;

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(display_RXsock, &sockRXbuffertemp[sockRXtail],512 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    writelog(LOG_DEBUG,"Got %d bytes from %s",numbytes, inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), a, sizeof a));
    switch (sockRXbuffertemp[0]) {
        case 0x80:    writelog(LOG_DEBUG,"Origin : serial passthrough"); break;
        case 0x90:    writelog(LOG_DEBUG,"Origin : transparent data"); break;
        default : writelog(LOG_DEBUG,"Origin unknown (%02X)",sockRXbuffertemp[0]); break;
    }
    buf[0]=0;
    unsigned int i;
    for (i=sockRXtail;i<(sockRXtail+numbytes);i++) {
        if (strlen(buf)<sizeof(buf)-10) {
            if ((sockRXbuffertemp[i]<32)||(sockRXbuffertemp[i]>126)) 
                sprintf(buf,"%s[%02X]",buf,sockRXbuffertemp[i]); 
                else sprintf(buf,"%s%c",buf,sockRXbuffertemp[i]); 
            } else {
                sprintf(buf,"%s ...",buf);
                break;
            }
    }
    writelog(LOG_DEBUG," data=%s",buf);

    if (numbytes>0) {
        sockRXtail+=numbytes;
        sockRXbuffertemp[sockRXtail]=0;

        s=strstr(sockRXbuffertemp,ENDMRKR);
        while (s!=NULL) {
//            printf("Found %s\n",sockRXbuffertemp+1);
            s[0]=0;
            memcpy(RXbuffer,sockRXbuffertemp+1,strlen(sockRXbuffertemp));
            handleButton(strlen(sockRXbuffertemp)-1);
            sockRXtail-=(strlen(sockRXbuffertemp)+3);
            memmove(&sockRXbuffertemp,&s[3],512);
            s=strstr(sockRXbuffertemp,ENDMRKR);
        }
    }
    if (sockRXtail>100) sockRXtail=0;
}


void talkToNextion(void) {
    if (strlen(TXbuffer)>0) writelog(LOG_DEBUG,"  RX: %s",TXbuffer);  //KE7FNS fix spacing in log
    basicFunctions();
    processCommands();
    sendCommand(TXbuffer);
    checkSerial();
    checkListeningSocket();
    if (!become_daemon) fflush(NULL);
}


void showRXbuffer(int buflen) {
    int z;
    if (!become_daemon) printf("RX:   ");
    for (z=0; z<buflen;z++) {
        if ((RXbuffer[z]<' ')||(RXbuffer[z]>254)) {
            if (!become_daemon) printf("0x%02X ",RXbuffer[z]); 
        } else {
            if (!become_daemon) printf("%c ",RXbuffer[z]);
        }
    }
    if (!become_daemon)  {printf("\n");fflush(NULL);}
}


int open_nextion_serial_port(char* devicename, long BAUD)
{
    int fd,errnr;
    struct termios newtio;


    fd = open(devicename,O_RDWR | O_NOCTTY | O_NONBLOCK);
    errnr=errno;
    if (fd < 0) {
        writelog(LOG_ERR,"Cannot open %s (%s). Exiting.",devicename,strerror(errnr));
        exit(EXIT_FAILURE);
    }

    tcgetattr(fd, &newtio);

    cfsetispeed(&newtio,BAUD);
    cfsetospeed(&newtio,BAUD);

    newtio.c_cflag &= ~PARENB;   // geen pariteit
    newtio.c_cflag &= ~CSTOPB;   // 1 stopbit
    newtio.c_cflag &= ~CSIZE;    // wis databitsize
    newtio.c_cflag |=  CS8;      // 8 databits

    newtio.c_cflag &= ~CRTSCTS;  // geen HW flow Control
    newtio.c_cflag |= CREAD | CLOCAL;

    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;


    tcflush(fd, TCIOFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio)) != 0) {
        writelog(LOG_ERR,"Error in setting serial port attributes");
        exit(EXIT_FAILURE);
    }

//  printf("port %s (fd %i)\n",devicename,fd);

    return fd;
}


int ptym_open(char *pts_name, char *pts_name_s , int pts_namesz)
{
    char *ptr;
    int fdm;

    strncpy(pts_name, "/dev/ptmx", pts_namesz);
    pts_name[pts_namesz - 1] = '\0';

    fdm = posix_openpt(O_RDWR | O_NONBLOCK);
    if (fdm < 0)
        return(-1);
    if (grantpt(fdm) < 0) {
        close(fdm);
        return(-2);
    }
    if (unlockpt(fdm) < 0) {
        close(fdm);
        return(-3);
    }
    if ((ptr = ptsname(fdm)) == NULL) {
        close(fdm);
        return(-4);
    }
    strncpy(pts_name_s, ptr, pts_namesz);
    pts_name[pts_namesz - 1] = '\0';

    return(fdm);
}


static void go_daemon() {
    pid_t pid = 0;
    pid_t sid = 0;

    pid = fork();
    if (pid < 0)
    {  writelog(LOG_ERR,"fork failed! Exiting."); exit(EXIT_FAILURE); }
    if (pid > 0)
    {  // printf("process_id of child process %d \n", pid); 
        exit(EXIT_SUCCESS); }
    umask(0);
    sid = setsid();
    if(sid < 0)
    { exit(EXIT_FAILURE); }
    chdir("/tmp");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}


static void terminate(int sig)
{
    char *signame[]={"INVALID", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPOLL", "SIGPWR", "SIGSYS", NULL};

    inhibit=0;
    sendCommand("sleep=0");
    sendCommand("ussp=0");
    sendCommand("page 0");
    sendCommand("cls 0");
    sendCommand("dim=50");
    sendCommand("t0.txt=\"NextionDriver\"");
    sprintf(TXbuffer,"t1.txt=\"%s\"",NextionDriver_VERSION);
    sendCommand(TXbuffer);
    sendCommand("t2.txt=\"Stopped\"");
    usleep(5000);

    writelog(LOG_ERR, "NextionDriver V%s terminated on signal %s (%s)",NextionDriver_VERSION,signame[sig],strsignal(sig));
    close(fd1);
    close(fd2);
    unlink(nextionDriverLink);
    exit(0);
}


int openTransparentDataPorts(void) {
    int ok;

    writelog(LOG_NOTICE,"Opening sockets ...");
    ok=openTalkingSocket();
    if (ok==1) ok=openListeningSocket();
    writelog(2,"Transparent data sockets%s active", transparentIsEnabled ? "":" NOT");
    return ok;
}



int main(int argc, char *argv[])
{
    int t,ok,wait;

    display_addr=0;
    check=0;
    gelezen=0;
    screenLayout=2;
    inhibit=0;

    become_daemon=TRUE;
    ignore_other=FALSE;
    ok=0;
    verbose=0;


    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // terminate on these signals
    signal(SIGINT, terminate);            // 'Ctrl-C'
    signal(SIGQUIT, terminate);           // 'Ctrl-\'
    signal(SIGHUP, terminate);
    signal(SIGTERM, terminate);
    signal(SIGUSR1, terminate);
    signal(SIGUSR2, terminate);

    strcpy(nextionDriverLink,NEXTIONDRIVERLINK);
    strcpy(nextionPort,NEXTIONPORT);
    strcpy(configFile,CONFIGFILE);
    strcpy(groupsFile,GROUPSFILE);
    strcpy(usersFile,USERSFILE);

    datafiledir[0]=0;
    for (t=1;t<8;t++)DisplayInfo[t][0]=0;

    while ((t = getopt(argc, argv, "dVvm:n:c:f:hi")) != -1) {
        switch (t) {
            case 'd':
                become_daemon = FALSE;
                break;
            case 'V':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf("Copyright (C) 2017...2019 ON7LDS. All rights reserved.\n\n");
                return 0;
                break;
            case 'v':
                if (verbose<4) verbose++;
                break;
            case 'm':
                strncpy(nextionDriverLink,optarg,sizeof(nextionDriverLink)-2);
                ok++;
                break;
            case 'n':
                strncpy(nextionPort,optarg,sizeof(nextionPort)-2);
                ok++;
                break;
            case 'c':
                strncpy(configFile,optarg,sizeof(configFile)-2);
                break;
            case 'f':
                strncpy(datafiledir,optarg,sizeof(datafiledir)-2);
                break;
            case 'i':
                ignore_other = TRUE;
                break;
            case 'h':
            case ':':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf("Copyright (C) 2017...2019 ON7LDS. All rights reserved.\n");
                printf("\nUsage: %s -c <MMDVM config file> [-f] [-d] [-h]\n\n", argv[0]);
                printf("  -c\tspecify the MMDVM config file, which has to be extended with the NetxtionDriver config\n");
                printf("  -f\tspecify the directory with data files (groups, users)\n");
                printf("  -d\tstart in debug mode (do not go to backgroud and print communication data)\n");
                printf("  -i\tignore other instances and run anyway\n");
                printf("  -v\tverbose (when running in daemon mode, dumps logging to syslog)\n");
                printf("  -V\tdisplay version and exit\n");
                printf("  -h\tthis help.\n\n");
                printf("Note: more options are possible by just specifying the configuration file.\n");
                printf(" All settings in the configuration file take priority over the options on the commandline.\n\n\n");
                exit(EXIT_SUCCESS);
                break;

        }
    }

    if (become_daemon) {
        go_daemon();
    } else printf("Starting in console mode...\n");

    writelog(2,"NextionDriver version %s", NextionDriver_VERSION);
    writelog(2,"Copyright (C) 2017...2019 ON7LDS. All rights reserved.");

    writelog(2,"Starting with verbose level %d", verbose);

    ok=proc_find("NextionDriver");
    if (ok>0) {
        if (ignore_other) {
            writelog(2,"Warning: NextionDriver with PID %d already running !",ok);
        } else {
            writelog(LOG_ERR,"ERROR: NextionDriver with PID %d already running, I'm quitting !",ok);
            exit(EXIT_FAILURE);
        }
    }

    if (!readConfig()) { writelog(LOG_ERR,"MMDVM Config not found. Exiting."); exit(EXIT_FAILURE); };

    writelog(2,"Started with screenLayout %d", screenLayout);
    writelog(2,"Started with verbose level %d", verbose);
    if (removeDim) writelog(2,"Dim commands will be removed");
    if (sleepWhenInactive) writelog(2,"Display will sleep when no data received for %d seconds",sleepWhenInactive);

    writelog(2,"Opening ports");  //KE7FNS moved to earlier on in the code to avoid a racing condition on RPi Zero W (single core) processors.
    fd1=ptym_open(mux,mmdvmPort,sizeof(mux));
    if (strcmp(nextionPort,"modem")!=0) {
        if (screenLayout==4) {
            fd2=open_nextion_serial_port(nextionPort,BAUDRATE4);
        } else {
            fd2=open_nextion_serial_port(nextionPort,BAUDRATE3);
        }
    } else fd2=-1;

    ok=unlink(nextionDriverLink);
//    if (ok!=?) { writelog(LOG_ERR,"Link %s could not be removed (%d). Exiting.",nextionDriverLink,ok); exit(EXIT_FAILURE); }
    ok=symlink(mmdvmPort, nextionDriverLink);
    if (ok!=0) { writelog(LOG_ERR,"Link %s could not be created (%d). Exiting.",nextionDriverLink,ok); exit(EXIT_FAILURE); }

    if (fd2>=0) {
        writelog(2," %s (=%s) <=> %s",nextionDriverLink,mmdvmPort,nextionPort);
    } else {
        writelog(2," %s (=%s) <=> modem",nextionDriverLink,mmdvmPort);
        if ((transparentIsEnabled==0)||(sendFrameType==0)) { 
            writelog(LOG_ERR,"Unable to start. For using a display connected to the modem,"); 
            writelog(LOG_ERR," you have to enable 'Transparent Data' and 'sendFrameType' !"); 
            exit(EXIT_FAILURE); 
        }
    }

    if (strlen(datafiledir)<3) {   //  KE7FNS  moved to a later time to allow the port to be open sooner so that it doesn't cause issues on RPi Zero W (single core) processors.
        ssize_t len;
        if ((len = readlink("/proc/self/exe", datafiledir, sizeof(datafiledir)-1)) != -1) {
            while ((strlen(datafiledir)>0)&&(datafiledir[strlen(datafiledir)-1]!='/')) {
                datafiledir[strlen(datafiledir)-1]=0;
            }
        }
    }
    writelog(4,"Data files directory: %s", datafiledir);

    readGroups();
    readUserDB();

    getDiskFree(TRUE);

    t=0; wait=0; int r=0;
    #define SERBUFSIZE 1024
    char buffer[SERBUFSIZE*2];
    char* s;
    int start=0, transparentIsOpen=0;

    if (transparentIsEnabled) transparentIsOpen=openTransparentDataPorts();

    int flash=0;
    char model[40];

    flash=checkDisplay(model);
    if (flash==0) writelog(LOG_ERR,"No Nextion display found.");

    sendCommand("bkcmd=0");        //  KE7FNS  force Nextion to not report status results to clean up serial repeater traffic to make it parallel to MMDVMHost setup
    sendCommand("sleep=0");
    sendCommand("page 0");
    sendCommand("cls 0");
    sendCommand("dim=100");
    sendCommand("t0.txt=\"NextionDriver\"");
    sprintf(TXbuffer,"t1.txt=\"%s\"",NextionDriver_VERSION);
    sendCommand(TXbuffer);
    sendCommand("t2.txt=\"Started\"");
    sprintf(TXbuffer,"ussp=%d",sleepWhenInactive);
    sendCommand(TXbuffer);
    sendCommand("thup=1");

    start=0;
    netIsActive[0]=0;
    if (waitForLan) {
        while ((start<180) && (netIsActive[0]==0)) {
            sleep(1);
            getNetworkInterface(ipaddr);
            netIsActive[0]=getInternetStatus(check);
            if (netIsActive[0])
                sprintf(TXbuffer,"t3.txt=\"%d %s\"", start, ipaddr);
            else
                sprintf(TXbuffer,"t3.txt=\"%d Waiting for network ...\"", start);
            sendCommand(TXbuffer);
            start++;
        }
        sprintf(TXbuffer,"t3.txt=\"%s\"", ipaddr);
        sendCommand(TXbuffer);
    }
    writelog(2,"Starting %s network interface %s", netIsActive[0] ? "with" : "without", netIsActive[0] ? ipaddr : "");

    //if needed, try again
    if (transparentIsEnabled && !transparentIsOpen) {
        writelog(2,"Retry to open sockets");
        start=0;
        while ((start<30) && !transparentIsOpen) {
            transparentIsOpen=openTransparentDataPorts();
            sleep(5);
            start+=5;
        }
    }

    start=0;
    RXtail=0;
    while(1)
    {
        if (start>SERBUFSIZE) start=0;
        r = read (fd1,&buffer[start],SERBUFSIZE);
        if ((r>0)&&(inhibit)) {
            writelog(LOG_DEBUG,"Inhibit %d: dropping %d bytes from host",inhibit,r);
            r=0;
            start=0;
            memset(&buffer,0,SERBUFSIZE);
        }
        if (r>0) {
            writelog(LOG_DEBUG,"Received %d bytes from host",r);
            memset(&buffer[start+r],0,SERBUFSIZE);
            start+=r;
        }
        s=strstr(buffer,ENDMRKR);
        while (s!=NULL) {
            s[0]=0;
            strcpy(TXbuffer,buffer);
            memmove(&buffer,&s[3],SERBUFSIZE);
            start-=(strlen(TXbuffer)+3);
            wait=0;
            s=strstr(buffer,ENDMRKR);
//            writelog(LOG_DEBUG,"Process %s",TXbuffer);
            talkToNextion();
        }
        if ((start==0)||(wait++>1000)) { wait=0; memset(buffer,0,SERBUFSIZE*2); TXbuffer[0]=0; talkToNextion(); }
//        usleep(1000);
    }
    close(fd1);
    close(fd2);
    unlink(nextionDriverLink);
    return EXIT_SUCCESS;
}
