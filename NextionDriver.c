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
        write(fd2,cmd,strlen(cmd));
        write(fd2,"\xFF\xFF\xFF",3);
        sendTransparentData(MMDVM_DISPLAY,cmd);
        writelog(LOG_DEBUG," TX:  %s",cmd);
    }
    if (!become_daemon)fflush(NULL);
}


void handleButton(int received) {
    char code, text[150];
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
//            if (!become_daemon) { printf("Received command %d\n",RXbuffer[1]); fflush(NULL); }
            writelog(LOG_DEBUG,"Received command 0x%02X\n",RXbuffer[1]);
            if (RXbuffer[1]>239) {
                    if ((received>2)&&(received<200)) {
//                        if (!become_daemon) { printf("Received command text \"%s\"\n",&RXbuffer[2]); fflush(NULL); }
                            writelog(LOG_DEBUG," Execute command \"%s\"",&RXbuffer[2]);
                            sprintf(TXbuffer, "msg.txt=\"Execute %s\"", &RXbuffer[2]);
                            sendCommand(TXbuffer);
                            if (RXbuffer[1]==241) {
                                FILE *ls = popen(&RXbuffer[2], "r");
                                char buf[256];
                                if (fgets(buf, sizeof(buf), ls) != 0) {
                                    strtok(buf, "\n");
                                    writelog(LOG_DEBUG," Command response \"%s\"",buf);
                                    sprintf(TXbuffer, "msg.txt=\"%s\"", buf);
                                    sendCommand(TXbuffer);
                                }
                                pclose(ls);
                            } else {
                                system(&RXbuffer[2]);
                            }
                    }
            } else {
                code=RXbuffer[1];
                memmove(&RXbuffer,&RXbuffer[2],512);
                writelog(LOG_DEBUG," Command parameter \"%s\"",RXbuffer);
                processButtons(code);
            }
        }
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

    int r = read (fd2,&RXbuffertemp[RXtail],512);
    if (r>0) {
//        writelog(LOG_DEBUG,"Receive %d bytes from serial",r);
        RXtail+=r;
        RXbuffertemp[RXtail]=0;

        s=strstr(RXbuffertemp,ENDMRKR);
        while (s!=NULL) {
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
#define MAXBUFLEN 100
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[4*MAXBUFLEN];
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
    for (i=sockRXtail;i<(sockRXtail+numbytes);i++) if ((sockRXbuffertemp[i]<32)||(sockRXbuffertemp[i]>126)) 
        sprintf(buf,"%s[%02X]",buf,sockRXbuffertemp[i]); else sprintf(buf,"%s%c",buf,sockRXbuffertemp[i]); 
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
    if (strlen(TXbuffer)>0) writelog(LOG_DEBUG,"RX:   %s",TXbuffer);
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
    int fd;
    struct termios newtio;


    fd = open(devicename,O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0) {
        perror(devicename);
        writelog(LOG_ERR,"Serial port %s not found. Exiting.",devicename);
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

    writelog(LOG_ERR, "NextionDriver V%s terminated on signal %s (%s)",NextionDriver_VERSION,signame[sig],strsignal(sig));
    close(fd1);
    close(fd2);
    unlink(nextionDriverLink);
    exit(0);
}


int main(int argc, char *argv[])
{
    int t,ok,wait;

    display_addr=0;
    check=1000;
    gelezen=0;
    screenLayout=2;

    become_daemon=TRUE;
    ok=0;
    verbose=0;


    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // terminate on these signals
    signal(SIGINT, terminate);            // 'Ctrl-C'
    signal(SIGQUIT, terminate);            // 'Ctrl-\'
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

    while ((t = getopt(argc, argv, "dVvm:n:c:f:h")) != -1) {
        switch (t) {
            case 'd':
                become_daemon = FALSE;
                break;
            case 'V':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf("Copyright (C) 2017,2018 ON7LDS. All rights reserved.\n\n");
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
            case 'h':
            case ':':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf("Copyright (C) 2017,2018 ON7LDS. All rights reserved.\n");
                printf("\nUsage: %s -c <MMDVM config file> [-f] [-d] [-h]\n\n", argv[0]);
                printf("  -c\tspecify the MMDVM config file, which has to be extended with the NetxtionDriver config\n");
                printf("  -f\tspecify the directory with data files (groups, users)\n");
                printf("  -d\tstart in debug mode (do not go to backgroud and print communication data)\n");
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
    writelog(2,"Copyright (C) 2017,2018 ON7LDS. All rights reserved.");

    writelog(2,"Starting with verbose level %d", verbose);

    if (!readConfig()) { writelog(LOG_ERR,"MMDVM Config not found. Exiting."); exit(EXIT_FAILURE); };

    if (strlen(datafiledir)<3) {
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

    writelog(2,"Started with screenLayout %d", screenLayout);
    writelog(2,"Started with verbose level %d", verbose);

    writelog(2,"Opening ports");
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

    t=0; wait=0; int r=0;
    #define SERBUFSIZE 1024
    char buffer[SERBUFSIZE*2];
    char* s;
    int start=0;

    if (transparentIsEnabled==1) transparentIsEnabled=openTalkingSocket();
    if (transparentIsEnabled==1) transparentIsEnabled=openListeningSocket();
    writelog(2,"Transparent data sockets%s active", transparentIsEnabled ? "":" NOT");

    RXtail=0;
    while(1)
    {
        if (start>SERBUFSIZE) start=0;
        r = read (fd1,&buffer[start],SERBUFSIZE);
        if (r>0) {
//            writelog(LOG_DEBUG,"Received %d bytes from host",r);
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
