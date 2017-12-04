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
#include "NextionDriver.h"
#include "processCommands.h"
#include "processButtons.h"


const char ENDMRKR[3]="\xFF\xFF\xFF";
int RXtail=0;
char RXbuffertemp[1024];

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

    if ( become_daemon==TRUE ) {
        str[98]='.';str[99]='.';str[100]='.'; str[101]=0; syslog(level, str);
    } else {
        printf("%s\n",str);
    }
}


void sendCommand(char *cmd){
    if (strlen(cmd)>0) {
        write(fd2,cmd,strlen(cmd));
        write(fd2,"\xFF\xFF\xFF",3);
        if (!become_daemon) { printf("TX: %s\n",cmd); }
    }
    if (!become_daemon)fflush(NULL);
}


void handleButton(int received) {
    char code;
    if (received>1) {
        if (!become_daemon) {
            printf("RX: %d (",received); int i; for (i=0; i<received; i++) printf("%02X ",RXbuffer[i]); printf(")\n"); 
        }
        if (RXbuffer[0]==42) {
            if (!become_daemon) { printf("Received command %d\n",RXbuffer[1]); fflush(NULL); }
            if (RXbuffer[1]>239) {
                    if ((received>2)&&(received<200)) {
                        if (!become_daemon) { printf("Received command text \"%s\"\n",&RXbuffer[2]); fflush(NULL); }
                            sprintf(TXbuffer, "msg.txt=\"Execute %s\"", &RXbuffer[2]);
                            sendCommand(TXbuffer);
                            if (RXbuffer[1]==241) {
                                FILE *ls = popen(&RXbuffer[2], "r");
                                char buf[256];
                                if (fgets(buf, sizeof(buf), ls) != 0) {
                                    strtok(buf, "\n");
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
                processButtons(code);
            }
        }
    }
}



void checkSerial(void) {
/*    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd2, &fds);

    struct timeval t = {0, 10000};
    int sel=select(fd2 + 1, &fds, NULL, NULL, &t);
    if (sel<=0) return;
*/
    char* s;

    int r = read (fd2,&RXbuffertemp[RXtail],512);
    if (r>0) {
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
}



void talkToNextion(void) {
    processCommands();
    sendCommand(TXbuffer);
    checkSerial();
    if (!become_daemon) fflush(NULL);
}


void showRXbuffer(int buflen) {
    int z;
    if (!become_daemon) printf("RX: ");
    for (z=0; z<buflen;z++) {
        if ((RXbuffer[z]<' ')||(RXbuffer[z]>254)) {
            if (!become_daemon) printf("0x%02X ",RXbuffer[z]); 
        } else {
            if (!become_daemon) printf("%c ",RXbuffer[z]);
        }
    }
    if (!become_daemon)  {printf("\n");fflush(NULL);}
}




int open_input_source(char* devicename, long BAUD)
{
    struct termios newtio;
    long DATABITS;
    long STOPBITS;
    long PARITYON;
    long PARITY;
    int  fd;

    DATABITS = CS8;
    STOPBITS = 0;	// 0 = 1 stopbit
    PARITYON = 0;	// 0 = geen pariteit
    PARITY   = 0;

    fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        perror(devicename);
        writelog(LOG_ERR,"Serial port %s not found. Exiting.",devicename);
        exit(1);
    }
    newtio.c_cflag = BAUD | CRTSCTS | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;

    cfmakeraw(&newtio);

    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
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

    writelog(LOG_NOTICE, "NextionDriver V%s terminated on signal %s (%s)",NextionDriver_VERSION,signame[sig],strsignal(sig));
    close(fd1);
    close(fd2);
    unlink(NextionDriverLink);
    exit(0);
}


int main(int argc, char *argv[])
{
    int t,ok,wait;

    become_daemon=TRUE;
    ok=0;


    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // terminate on these signals
    signal(SIGHUP, terminate);
    signal(SIGTERM, terminate);
    signal(SIGUSR1, terminate);
    signal(SIGUSR2, terminate);

    strcpy(NextionDriverLink,NEXTIONDRIVERLINK);
    strcpy(nextionPort,NEXTIONPORT);

    while ((t = getopt(argc, argv, "dvm:n:h")) != -1) {
        switch (t) {
            case 'd':
                become_daemon = FALSE;
                break;
            case 'v':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf("Copyright (C) 2017 ON7LDS. All rights reserved.\n\n");
                return 0;
                break;
           case 'm':
                strncpy(NextionDriverLink,optarg,sizeof(nextionPort)-2);
                ok++;
                break;
           case 'n':
                strncpy(nextionPort,optarg,sizeof(nextionPort)-2);
                ok++;
                break;
           case 'h':
           case ':':
                printf("\nUsage: %s [-n <Nextion port serial port>] [-m <MMDVMHost port>] [-d] [-h]\n\n", argv[0]);
                printf("  -n\tspecify the serial port where the Nextion is connected\n");
                printf("    \t (default = %s)\n",NEXTIONDRIVERLINK);
                printf("  -m\tspecify the virtual serial port link that will be created to connect MMDVMHost to\n");
                printf("    \t (default = %s)\n",NEXTIONPORT);
                printf("  -d\tstart in debug mode (do not go to backgroud and print communication data)\n");
                printf("  -h\tthis help.\n\n");
                printf("Example : %s -n /dev/ttyAMA0 -m /dev/ttyNextionDriver\n\n", argv[0]);

                exit(0);
                break;

        }
    }

/*
    if (ok<2) {
        printf("ERROR: no ports given.\nTip: use -h\n");
        exit(1);
    }
*/
    if (become_daemon) {
        go_daemon();
    } else printf("Starting in console mode...\n");

    writelog(LOG_NOTICE,"NextionDriver version %s", NextionDriver_VERSION);
    writelog(LOG_NOTICE,"Copyright (C) 2017 ON7LDS. All rights reserved.");

    fd1=ptym_open(mux,mmdvmPort,sizeof(mux));
    fd2=open_input_source(nextionPort,BAUDRATE);
    ok=unlink(NextionDriverLink);
//    if (ok!=?) { writelog(LOG_ERR,"Link %s could not be removed (%d). Exiting.",NextionDriverLink,ok); exit(EXIT_FAILURE); }
    ok=symlink(mmdvmPort, NextionDriverLink);
    if (ok!=0) { writelog(LOG_ERR,"Link %s could not be created (%d). Exiting.",NextionDriverLink,ok); exit(EXIT_FAILURE); }
    writelog(LOG_NOTICE,"%s (=%s) <=> %s",NextionDriverLink,mmdvmPort,nextionPort);

    t=0; wait=0; int r=0;
    char buffer[1024];
    char* s;
    int start=0;

    RXtail=0;
    while(1)
    {
        r = read (fd1,&buffer[start],512);
        if (r>0) {
            memset(&buffer[start+r],0,511);
            start+=r;
        }
        s=strstr(buffer,ENDMRKR);
        while (s!=NULL) {
            s[0]=0;
            strcpy(TXbuffer,buffer);
            memmove(&buffer,&s[3],512);
            start-=(strlen(TXbuffer)+3);
            wait=0;
            s=strstr(buffer,ENDMRKR);
            talkToNextion();
        }
        if (wait++>1000) { wait=0; TXbuffer[0]=0; talkToNextion(); }
    }
    close(fd1);
    close(fd2);
    unlink(NextionDriverLink);
    return EXIT_SUCCESS;
}
