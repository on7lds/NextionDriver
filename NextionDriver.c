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
#include "NextionDriver.h"
#include "processCommands.h"

int open_input_source(char* devicename, long snelheid)
{
    struct termios newtio;
    long BAUD;
    long DATABITS;
    long STOPBITS;
    long PARITYON;
    long PARITY;
    int  fd;

    BAUD     = snelheid;
    DATABITS = CS8;
    STOPBITS = 0;	// 0 = 1 stopbit
    PARITYON = 0;	// 0 = geen pariteit
    PARITY   = 0;

    fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        perror(devicename);
        printf("Serial port %s not found.\n\n",devicename);
        exit(1);
    }
    newtio.c_cflag = BAUD | CRTSCTS | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;

    cfmakeraw(&newtio);

    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
    printf("port %s (fd %i)\n",devicename,fd);
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
    {  printf("fork failed!\n"); exit(EXIT_FAILURE); }
    if (pid > 0)
    {  printf("process_id of child process %d \n", pid); exit(EXIT_SUCCESS); }
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

    syslog(LOG_NOTICE, "NextionDriver V%s terminated on signal %s (%s)",NextionDriver_VERSION,signame[sig],strsignal(sig));
    close(fd1);
    close(fd2);
    unlink(NextionDriverLink);
    exit(0);
}


int main(int argc, char *argv[])
{
    char c1;
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

    while ((t = getopt(argc, argv, "dvm:n:h")) != -1) {
        switch (t) {
            case 'd':
                become_daemon = FALSE;
                break;
            case 'v':
                printf("\nNextionDriver version %s\n", NextionDriver_VERSION);
                printf ("Copyright (C) 2017 ON7LDS. All rights reserved.\n\n");
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
                printf("\nUsage: %s -n <Nextion port serial port> -m <MMDVMHost port> [-d] [-h]\n\n", argv[0]);
                printf("  -n\tspecify the serial port where the Nextion is connected\n");
                printf("  -m\tspecify the virtual serial port link that will be created to connect MMDVMHost to\n");
                printf("  -d\tstart in debug mode (do not go to backgroud and print communication data)\n");
                printf("  -h\tthis help.\n\n");
                printf("Example : %s -n /dev/ttyAMA0 -m /dev/NextionDriver\n\n", argv[0]);

                exit(0);
                break;

        }
    }

    if (ok<2) {
        printf("ERROR: no ports given.\nTip: use -h\n");
        exit(1);
    }

    if (become_daemon) {
        go_daemon();
        syslog(LOG_NOTICE, "NextionDriver version %s", NextionDriver_VERSION);
        syslog(LOG_NOTICE, "Copyright (C) 2017 ON7LDS. All rights reserved.");
    } else printf("Started in console mode.\n");


    fd1=ptym_open(mux,mmdvmPort,sizeof(mux));
    fd2=open_input_source(nextionPort,9600);
    unlink(NextionDriverLink);
    symlink(mmdvmPort, NextionDriverLink);

    if (become_daemon)
        syslog(LOG_NOTICE, "%s (=%s) <=> %s\n",NextionDriverLink,mmdvmPort,nextionPort);
    else
        printf("%s (=%s) <=> %s\n",NextionDriverLink,mmdvmPort,nextionPort);

    t=0; wait=0;
    while(1)
    {
    // MMDVMHost to Nextion commands
        while(read (fd1,&c1,1) == 1) {
            TXbuffer[t++]=c1; TXbuffer[t]=0;
            if ((t>3)&&(TXbuffer[t-1]==255)&&(TXbuffer[t-2]==255)&&(TXbuffer[t-3]==255)) {
                //command found, do something with it
                TXbuffer[t-3]=0; t=0; process();
                wait=0;
                break;
            }
        }
        usleep(1000);
        if (wait++>500) { wait=0; TXbuffer[0]=0; process(); }
    }
    close(fd1);
    close(fd2);
    unlink(NextionDriverLink);
    return EXIT_SUCCESS;
}
