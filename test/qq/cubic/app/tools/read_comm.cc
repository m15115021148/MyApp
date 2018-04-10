#include "cubic_inc.h"
#include "CLogger.cc"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "cubic_func.h"
#include <sys/time.h>
#include <termios.h>
#include <string>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/un.h>

#include <sys/stat.h>
#include <sstream>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <syscall.h>
#include <assert.h>
#include <cctype>


#define MAX_PORTS             4
#define RECV_TIMEOUT          1000
#define TIMER_TIMEOUT         3000
#define LOC_LENGTH_MAX        256
#define TIMER_COMMON          200
#define MAX_COMMAND_STR_LEN   200
#define MAX_BUF_LENGTH        65526
using namespace std;

static int s_stop_flag = 0;

static void quit_comm( int sig )
{
    switch( sig )
    {
    case SIGINT:
    case SIGHUP:
    case SIGABRT:
    case SIGTERM:
    case SIGSTOP:
        s_stop_flag = 1;
        break;

    case SIGCHLD:
        break;
    }
};

typedef struct PortInfo
{
    int busy;
    char name[32];
    int handle;
} PortInfo;

int main( int argc, const char *argv[] )
{
    printf( "start \n" );
    signal( SIGINT,  quit_comm );
    signal( SIGHUP,  quit_comm );
    signal( SIGABRT, quit_comm );
    signal( SIGTERM, quit_comm );
    signal( SIGSTOP, quit_comm );
    signal( SIGCHLD, quit_comm );
    int BaudR = B115200;

    switch( argc )
    {
    case 115200:
        BaudR = B115200;
        break;

    case 57600:
        BaudR = B57600;
        break;

    case 19200:
        BaudR = B19200;
        break;

    case 9600:
        BaudR = B9600;
        break;

    default:
        BaudR = B0;
        break;
    }

    PortInfo ports[MAX_PORTS];
    int portno = 0;
    struct termios newtio;
    ports[portno].busy = 1;
    char deviceName[20];
    memset( deviceName, 0, sizeof( deviceName ) );
    sprintf( deviceName, "%s", argv[2] );

    if ( ( ports[portno].handle = open( deviceName, O_RDONLY ) ) == -1 )
    {
        printf( "open fail\n" );
        return -1;
    }

    printf( "open %s success\n", deviceName );
    newtio.c_cflag = CS8 | CLOCAL | CREAD ;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VINTR]    = 0;
    newtio.c_cc[VQUIT]    = 0;
    newtio.c_cc[VERASE]   = 0;
    newtio.c_cc[VKILL]    = 0;
    newtio.c_cc[VEOF]     = 4;
    newtio.c_cc[VTIME]    = 0;
    newtio.c_cc[VMIN]     = 1;
    newtio.c_cc[VSWTC]    = 0;
    newtio.c_cc[VSTART]   = 0;
    newtio.c_cc[VSTOP]    = 0;
    newtio.c_cc[VSUSP]    = 0;
    newtio.c_cc[VEOL]     = 0;
    newtio.c_cc[VREPRINT] = 0;
    newtio.c_cc[VDISCARD] = 0;
    newtio.c_cc[VWERASE]  = 0;
    newtio.c_cc[VLNEXT]   = 0;
    newtio.c_cc[VEOL2]    = 0;
    cfsetospeed( &newtio, BaudR );
    cfsetispeed( &newtio, BaudR );
    tcsetattr( ports[portno].handle, TCSANOW, &newtio );
    char buf[MAX_BUF_LENGTH];
    memset( buf, 0, sizeof( buf ) );

    while( s_stop_flag == 0 )
    {
        read( ports[portno].handle, buf , sizeof( buf ) );
        printf( "read_comm result : %s \n", buf );
    };

    close( ports[portno].handle );

    return 0;
};

